use crate::types::{Texture, NoiseTextureProvider, BlankTextureProvider, WorkerPoolProvider, GraphicsProvider};
use crate::context::DefaultContext;
use std::rc::Rc;
use rand;

pub struct DefaultChain<'a> {
    context: &'a DefaultContext<'a>,
    size: (u32, u32),
    noise_texture: Rc<Texture>,
}

impl<'a> DefaultChain<'a> {
    /// Construct a new chain for a given texture size
    pub fn new(context: &'a DefaultContext, size: (u32, u32)) -> DefaultChain<'a> {
        let texture_size = wgpu::Extent3d {
            width: size.0,
            height: size.1,
            depth: 1,
        };
        let texture = context.graphics().device.create_texture(
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

        context.graphics().queue.write_texture(
            wgpu::TextureCopyView {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
            },
            &random_bytes,
            wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: 4 * size.0,
                rows_per_image: size.1,
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = context.graphics().device.create_sampler(
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

        DefaultChain {
            context: context,
            size: size,
            noise_texture: Rc::new(Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }),
        }
    }
}

impl NoiseTextureProvider for DefaultChain<'_> {
    fn noise_texture(&self) -> Rc<Texture> {
        self.noise_texture.clone()
    }
}

impl BlankTextureProvider for DefaultChain<'_> {
    fn blank_texture(&self) -> Rc<Texture> {
        self.context.blank_texture()
    }
}

impl<'a> WorkerPoolProvider for DefaultChain<'a> {
    type Handle<T: Send + 'static> = <DefaultContext<'a> as WorkerPoolProvider>::Handle<T>;

    fn spawn<T: Send + 'static>(&self, f: fn () -> T) -> Self::Handle<T> {
        self.context.spawn(f)
    }
}
