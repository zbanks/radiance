use crate::types::{NoiseTexture, BlankTexture, GraphicsContext, Texture, WorkerPool, Graphics, FetchContent};
use crate::threaded_worker::ThreadWorkHandle;
use wgpu;
use std::rc::Rc;
use rand;
use std::collections::HashMap;
use std::fs::read_to_string;

#[derive(Debug)]
pub struct DefaultContext {
    chains: HashMap<u32, DefaultChain>,
    chain_id: u32,
    graphics: Rc<GraphicsContext>,
    blank_texture: Rc<Texture>,
}

impl DefaultContext {
    pub fn new(graphics: Rc<GraphicsContext>) -> DefaultContext {
        let tex = DefaultContext::create_blank_texture(&graphics);

        DefaultContext {
            chains: HashMap::new(),
            graphics: graphics,
            blank_texture: tex,
            chain_id: 0,
        }
    }

    fn create_blank_texture(graphics: &GraphicsContext) -> Rc<Texture> {
        // Create blank texture
        let texture_size = wgpu::Extent3d {
            width: 1,
            height: 1,
            depth: 1,
        };
        let texture = graphics.device.create_texture(
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

        graphics.queue.write_texture(
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
        let sampler = graphics.device.create_sampler(
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

        Rc::new(Texture {
            texture: texture,
            view: view,
            sampler: sampler,
        })
    }

    pub fn add_chain(&mut self, size: (u32, u32)) -> u32 {
        let chain = DefaultChain::new(self.graphics.as_ref(), size, self.blank_texture.clone());
        let id = self.chain_id;
        self.chain_id += 1;
        self.chains.insert(id, chain);
        id
    }

    pub fn chain(&self, id: u32) -> Option<&DefaultChain> {
        self.chains.get(&id)
    }
}

#[derive(Debug)]
pub struct DefaultChain {
    size: (u32, u32),
    blank_texture: Rc<Texture>,
    noise_texture: Rc<Texture>,
}

impl DefaultChain {
    /// Construct a new chain for a given texture size
    pub fn new(graphics: &GraphicsContext, size: (u32, u32), blank_texture: Rc<Texture>) -> DefaultChain {
        let texture_size = wgpu::Extent3d {
            width: size.0,
            height: size.1,
            depth: 1,
        };
        let texture = graphics.device.create_texture(
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

        graphics.queue.write_texture(
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
        let sampler = graphics.device.create_sampler(
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
            size: size,
            noise_texture: Rc::new(Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }),
            blank_texture: blank_texture,
        }
    }
}

impl BlankTexture for DefaultChain {
    fn blank_texture(&self) -> Rc<Texture> {
        self.blank_texture.clone()
    }
}

impl NoiseTexture for DefaultChain {
    fn noise_texture(&self) -> Rc<Texture> {
        self.noise_texture.clone()
    }
}

impl WorkerPool for DefaultContext {
    type Handle<T: Send + 'static> = ThreadWorkHandle<T>;

    fn spawn<T: Send + 'static, F: FnOnce () -> T + Send + 'static>(&self, f: F) -> ThreadWorkHandle<T> {
        ThreadWorkHandle::new(f)
    }
}

impl Graphics for DefaultContext {
    fn graphics(&self) -> Rc<GraphicsContext> {
        self.graphics.clone()
    }
}

impl FetchContent for DefaultContext {
    fn fetch_content_closure(&self, name: &str) -> Box<dyn FnOnce() -> std::io::Result<String> + Send + 'static> {
        let cloned_name = name.to_string();
        return Box::new(move || {
            read_to_string(cloned_name)
        })
    }
}
