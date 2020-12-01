use crate::types::{BlankTextureProvider, GraphicsContext, Texture, WorkerPoolProvider};
use crate::chain::DefaultChain;
use wgpu;
use std::rc::Rc;
use futures::executor::ThreadPool;
use futures::task::SpawnExt;
use futures::future::RemoteHandle;

pub struct DefaultContext {
    chains: Vec<Rc<DefaultChain>>,
    graphics: Rc<GraphicsContext>,
    blank_texture: Rc<Texture>,
    worker_pool: ThreadPool,
}

impl DefaultContext {
    pub fn new(graphics: Rc<GraphicsContext>) -> DefaultContext {
        let tex = DefaultContext::create_blank_texture(&graphics);
        let pool = DefaultContext::create_worker_pool();

        DefaultContext {
            chains: Vec::new(),
            graphics: graphics,
            blank_texture: tex,
            worker_pool: pool,
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

    pub fn create_worker_pool() -> ThreadPool {
        ThreadPool::new().expect("Unable to create threadpool")
    }

    pub fn add_chain(&mut self, size: (u32, u32)) -> Rc<DefaultChain> {
        let chain = Rc::new(DefaultChain::new(&self.graphics, size));
        self.chains.push(chain.clone());
        chain
    }
}

impl BlankTextureProvider for DefaultContext {
    fn blank_texture(&self) -> Rc<Texture> {
        self.blank_texture.clone()
    }
}

//impl<T: Send + 'static> WorkerPoolProvider<T> for DefaultContext {
//    type Fut = RemoteHandle<T>;
//    fn spawn(&self, f: fn () -> T) -> RemoteHandle<T> {
//        self.worker_pool.spawn_with_handle(async move {f()}).expect("Could not spawn task")
//    }
//}

impl WorkerPoolProvider for DefaultContext {
    type Fut<O: Send + 'static> = RemoteHandle<O>;
    fn spawn<T: Send + 'static>(&self, f: fn () -> T) -> RemoteHandle<T> {
        self.worker_pool.spawn_with_handle(async move {f()}).expect("Could not spawn task")
    }
}
