use libradiance::{DefaultContext, DefaultChain};
use libradiance::GraphicsContext;
use futures::executor::block_on;
use std::rc::Rc;

fn main() {
    // Set up WGPU
    let adapter = block_on(wgpu::Instance::new(wgpu::BackendBit::PRIMARY).request_adapter(
        &wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::Default,
            compatible_surface: None,
        }
    )).unwrap();
    let (device, queue) = block_on(adapter.request_device(&Default::default(), None)).unwrap();
    let graphics = Rc::new(GraphicsContext {device: device, queue: queue});

    // Create a radiance Context
    let ctx = DefaultContext::new(graphics.clone());

    let texture_size = 256;
    //let test_chain = ctx.add_chain((texture_size, texture_size));
    let test_chain = DefaultChain::new(&graphics, (texture_size, texture_size));

    // Read out the noise texture, as a test
    let mut encoder = graphics.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
        label: None,
    });

    let output_buffer_size = (4 * texture_size * texture_size) as wgpu::BufferAddress;
    let output_buffer_desc = wgpu::BufferDescriptor {
        size: output_buffer_size,
        usage: wgpu::BufferUsage::COPY_DST
            | wgpu::BufferUsage::MAP_READ,
        label: None,
        mapped_at_creation: false,
    };
    let output_buffer = graphics.device.create_buffer(&output_buffer_desc);

    let texture_extent = wgpu::Extent3d {
        width: texture_size as u32,
        height: texture_size as u32,
        depth: 1,
    };

    encoder.copy_texture_to_buffer(
        wgpu::TextureCopyView {
            texture: &test_chain.noise_texture.texture,
            mip_level: 0,
            origin: wgpu::Origin3d::ZERO,
        }, 
        wgpu::BufferCopyView {
            buffer: &output_buffer,
            layout: wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: 4 * texture_size as u32,
                rows_per_image: texture_size as u32,
            },
        }, 
        texture_extent,
    );

    graphics.queue.submit(Some(encoder.finish()));

    // NOTE: We have to create the mapping THEN device.poll(). If we don't
    // the application will freeze.
    let buffer_slice = output_buffer.slice(..);
    let mapping = buffer_slice.map_async(wgpu::MapMode::Read);
    graphics.device.poll(wgpu::Maintain::Wait);

    block_on(mapping).unwrap();
    let data = buffer_slice.get_mapped_range();

    use image::{ImageBuffer, Rgba};
    let buffer = ImageBuffer::<Rgba<u8>, _>::from_raw(
        texture_size as u32,
        texture_size as u32,
        data,
    ).unwrap();

    buffer.save("image.png").unwrap();

    println!("Hello, world!");
}
