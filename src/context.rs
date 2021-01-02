use crate::types::{NoiseTexture, BlankTexture, Texture, WorkerPool, FetchContent, Resolution, Timebase, UniqueId};
use crate::threaded_worker::ThreadWorkHandle;
use wgpu;
use std::rc::Rc;
use rand;
use std::fs::read_to_string;
use std::time::{Instant, Duration};

#[derive(Debug)]
pub struct DefaultContext {
    blank_texture: Rc<Texture>,
    start_time: Option<Instant>,
    cur_time: Duration,
    next_chain_id: u32,
}

impl DefaultContext {
    pub fn new(device: &wgpu::Device, queue: &wgpu::Queue) -> DefaultContext {
        let tex = DefaultContext::create_blank_texture(device, queue);

        DefaultContext {
            blank_texture: tex,
            start_time: None,
            cur_time: Duration::new(0, 0),
            next_chain_id: 0,
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

    pub fn update(&mut self) {
        match self.start_time {
            None => {
                self.start_time = Some(Instant::now());
                self.cur_time = Duration::new(0, 0);
            },
            Some(st) => {
                self.cur_time = Instant::now().duration_since(st);
            },
        }
    }

    pub fn new_chain(&mut self, device: &wgpu::Device, queue: &wgpu::Queue, resolution: (u32, u32)) -> DefaultChain {
        let id = self.next_chain_id;
        self.next_chain_id += 1;
        DefaultChain::new(device, queue, resolution, self.blank_texture.clone(), id)
    }
}

#[derive(Debug)]
pub struct DefaultChain {
    resolution: (u32, u32),
    blank_texture: Rc<Texture>,
    noise_texture: Rc<Texture>,
    id: u32,
}

impl DefaultChain {
    /// Construct a new chain for a given texture resolution
    pub fn new(device: &wgpu::Device, queue: &wgpu::Queue, resolution: (u32, u32), blank_texture: Rc<Texture>, id: u32) -> DefaultChain {
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
            noise_texture: Rc::new(Texture {
                texture: texture,
                view: view,
                sampler: sampler,
            }),
            blank_texture,
            id,
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

impl Resolution for DefaultChain {
    fn resolution(&self) -> (u32, u32) {
        self.resolution
    }
}

impl UniqueId for DefaultChain {
    fn id(&self) -> u32 {
        self.id
    }
}

impl WorkerPool for DefaultContext {
    type Handle<T: Send + 'static> = ThreadWorkHandle<T>;

    fn spawn<T: Send + 'static, F: FnOnce () -> T + Send + 'static>(&self, f: F) -> ThreadWorkHandle<T> {
        ThreadWorkHandle::new(f)
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

impl Timebase for DefaultContext {
    fn time(&self) -> f32 {
        (2. * self.cur_time.as_secs_f64() % 16.) as f32
    }
}
