use crate::types::{BlankTextureProvider, GraphicsContext, Texture, WorkerPoolProvider, GraphicsProvider, WorkResult, WorkHandle};
use crate::chain::DefaultChain;
use wgpu;
use std::rc::Rc;
use std::sync::{Arc, Weak};
use std::sync::atomic::AtomicBool;
use std::thread;
use std::thread::JoinHandle;

pub struct DefaultContext<'a> {
    chains: Vec<DefaultChain<'a>>,
    graphics: Rc<GraphicsContext>,
    blank_texture: Rc<Texture>,
}

impl<'a> DefaultContext<'a> {
    pub fn new(graphics: Rc<GraphicsContext>) -> DefaultContext<'a> {
        let tex = DefaultContext::create_blank_texture(&graphics);

        DefaultContext {
            chains: Vec::new(),
            graphics: graphics,
            blank_texture: tex,
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

    pub fn add_chain(&'a mut self, size: (u32, u32)) -> () {
        //let chain = DefaultChain::new(&self, size);
        //self.chains.push(chain);
    }
}

impl BlankTextureProvider for DefaultContext<'_> {
    fn blank_texture(&self) -> Rc<Texture> {
        self.blank_texture.clone()
    }
}

pub struct ThreadWorkHandle<T> {
    handle: JoinHandle<T>,
    alive: Weak<AtomicBool>,
}

impl<T: Send + 'static> ThreadWorkHandle<T> {
    fn new(f: fn () -> T) -> Self {
        let alive = Arc::new(AtomicBool::new(true));
        let alive_weak = Arc::downgrade(&alive);

        let handle = thread::spawn(move || {
            let r = f();
            drop(alive); // Don't know if there's a better way to capture this value
            r
        });

        ThreadWorkHandle {
            alive: alive_weak,
            handle: handle,
        }
    }
}

impl<T: Send + 'static> WorkHandle for ThreadWorkHandle<T> {
    type Output = T;

    fn alive(&self) -> bool {
        match self.alive.upgrade() {
            Some(_) => true,
            None => false,
        }
    }

    fn join(self) -> WorkResult<T> {
        match self.handle.join() {
            thread::Result::Ok(v) => WorkResult::Ok(v),
            thread::Result::Err(_) => WorkResult::Err(()),
        }
    }
}

impl WorkerPoolProvider for DefaultContext<'_> {
    type Handle<T: Send + 'static> = ThreadWorkHandle<T>;

    fn spawn<T: Send + 'static>(&self, f: fn () -> T) -> ThreadWorkHandle<T> {
        ThreadWorkHandle::new(f)
    }
}

impl GraphicsProvider for DefaultContext<'_> {
    fn graphics(&self) -> Rc<GraphicsContext> {
        self.graphics.clone()
    }
}
