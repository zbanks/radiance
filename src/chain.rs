use crate::context::GraphicsContext;
use crate::context::Texture;
use rand;

pub struct Chain {
    size: (usize, usize),
    pub noise_texture: Texture, // XXX shouldn't be pub
}

impl Chain {
    /// Construct a new chain for a given texture size
    pub fn new(graphics_context: &GraphicsContext, size: (usize, usize)) -> Chain {
        let texture_size = wgpu::Extent3d {
            width: size.0 as u32,
            height: size.1 as u32,
            depth: 1,
        };
        let texture = graphics_context.device.create_texture(
            &wgpu::TextureDescriptor {
                size: texture_size,
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsage::SAMPLED | wgpu::TextureUsage::COPY_DST | wgpu::TextureUsage::COPY_SRC, // XXX remove COPY_SRC
                label: Some("noise texture"),
            }
        );

        let random_bytes: Vec<u8> = (0 .. size.0 * size.1 * 4).map(|_| { rand::random::<u8>() }).collect();

        graphics_context.queue.write_texture(
            wgpu::TextureCopyView {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
            },
            &random_bytes,
            wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: (4 * size.0) as u32,
                rows_per_image: size.1 as u32,
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = graphics_context.device.create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Nearest,
                ..Default::default()
            }
        );

        Chain {
            size: size,
            noise_texture: Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }
        }
    }
}
