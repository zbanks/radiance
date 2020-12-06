use crate::types::{NoiseTexture, BlankTexture, Graphics, Texture, WorkerPool, FetchContent, Resolution};
use crate::threaded_worker::ThreadWorkHandle;
use wgpu;
use std::rc::Rc;
use rand;
use std::collections::HashMap;
use std::fs::read_to_string;

#[derive(Debug)]
pub struct DefaultContext<'a> {
    chains: HashMap<u32, DefaultChain<'a>>,
    chain_id: u32,
    device: &'a wgpu::Device,
    queue: &'a wgpu::Queue,
    blank_texture: Rc<Texture>,
}

impl<'a> DefaultContext<'a> {
    pub fn new(device: &'a wgpu::Device, queue: &'a wgpu::Queue) -> DefaultContext<'a> {
        let tex = DefaultContext::create_blank_texture(device, queue);

        DefaultContext {
            chains: HashMap::new(),
            device,
            queue,
            blank_texture: tex,
            chain_id: 0,
        }
    }

    fn create_blank_texture(device: &wgpu::Device, queue: &wgpu::Queue) -> Rc<Texture> {
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

        Rc::new(Texture {
            texture: texture,
            view: view,
            sampler: sampler,
        })
    }

    pub fn add_chain(&mut self, resolution: (u32, u32)) -> u32 {
        let chain = DefaultChain::new(self.device, self.queue, resolution, self.blank_texture.clone());
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
pub struct DefaultChain<'a> {
    resolution: (u32, u32),
    device: &'a wgpu::Device,
    queue: &'a wgpu::Queue,
    blank_texture: Rc<Texture>,
    noise_texture: Rc<Texture>,
}

impl<'a> DefaultChain<'a> {
    /// Construct a new chain for a given texture resolution
    pub fn new(device: &'a wgpu::Device, queue: &'a wgpu::Queue, resolution: (u32, u32), blank_texture: Rc<Texture>) -> DefaultChain<'a> {
        let texture_size = wgpu::Extent3d {
            width: resolution.0,
            height: resolution.1,
            depth: 1,
        };
        let texture = device.create_texture(
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

        let random_bytes: Vec<u8> = (0 .. resolution.0 * resolution.1 * 4).map(|_| { rand::random::<u8>() }).collect();

        queue.write_texture(
            wgpu::TextureCopyView {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
            },
            &random_bytes,
            wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: 4 * resolution.0,
                rows_per_image: resolution.1,
            },
            texture_size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(
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
            resolution,
            device,
            queue,
            noise_texture: Rc::new(Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }),
            blank_texture,
        }
    }
}

impl BlankTexture for DefaultChain<'_> {
    fn blank_texture(&self) -> Rc<Texture> {
        self.blank_texture.clone()
    }
}

impl NoiseTexture for DefaultChain<'_> {
    fn noise_texture(&self) -> Rc<Texture> {
        self.noise_texture.clone()
    }
}

impl Resolution for DefaultChain<'_> {
    fn resolution(&self) -> (u32, u32) {
        self.resolution
    }
}

impl Graphics for DefaultChain<'_> {
    fn graphics_device(&self) -> &wgpu::Device {
        self.device
    }
    fn graphics_queue(&self) -> &wgpu::Queue {
        self.queue
    }
}

impl WorkerPool for DefaultContext<'_> {
    type Handle<T: Send + 'static> = ThreadWorkHandle<T>;

    fn spawn<T: Send + 'static, F: FnOnce () -> T + Send + 'static>(&self, f: F) -> ThreadWorkHandle<T> {
        ThreadWorkHandle::new(f)
    }
}

impl Graphics for DefaultContext<'_> {
    fn graphics_device(&self) -> &wgpu::Device {
        self.device
    }
    fn graphics_queue(&self) -> &wgpu::Queue {
        self.queue
    }
}

impl FetchContent for DefaultContext<'_> {
    fn fetch_content_closure(&self, name: &str) -> Box<dyn FnOnce() -> std::io::Result<String> + Send + 'static> {
        let cloned_name = name.to_string();
        return Box::new(move || {
            read_to_string(cloned_name)
        })
    }
}
