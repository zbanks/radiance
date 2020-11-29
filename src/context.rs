use crate::types::{BlankTextureProvider, GraphicsContext, Texture};
use crate::chain::DefaultChain;
use wgpu;
use std::rc::Rc;

pub struct DefaultContext<'a> {
    chains: Vec<Rc<DefaultChain>>,
    graphics_context: GraphicsContext<'a>,
    blank_texture: Rc<Texture>,
}

impl<'a> DefaultContext<'a> {
    pub fn new(device: &'a wgpu::Device, queue: &'a wgpu::Queue) -> DefaultContext<'a> {

        // Create blank texture
        let texture_size = wgpu::Extent3d {
            width: 1,
            height: 1,
            depth: 1,
        };
        let texture = device.create_texture(
            &wgpu::TextureDescriptor {
                size: texture_size,
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsage::SAMPLED | wgpu::TextureUsage::COPY_DST,
                label: Some("blank_texture"),
            }
        );

        queue.write_texture(
            wgpu::TextureCopyView {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
            },
            &[0, 0, 0, 0],
            wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: 4,
                rows_per_image: 1,
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(
            &wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Nearest,
                min_filter: wgpu::FilterMode::Nearest,
                mipmap_filter: wgpu::FilterMode::Nearest,
                ..Default::default()
            }
        );

        DefaultContext {
            chains: Vec::new(),
            graphics_context: GraphicsContext {
                device: device,
                queue: queue,
            },
            blank_texture: Rc::new(Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }),
        }
    }

    pub fn add_chain(&mut self, size: (usize, usize)) -> Rc<DefaultChain> {
        let chain = Rc::new(DefaultChain::new(&self.graphics_context, size));
        self.chains.push(chain.clone());
        chain
    }
}

impl BlankTextureProvider for DefaultContext<'_> {
    fn blank_texture(&self) -> Rc<Texture> {
        self.blank_texture.clone()
    }
}
