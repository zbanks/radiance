use std::rc::Rc;
use futures::Future;

/// Convenient packaging for a texture, view, and sampler
pub struct Texture {
    pub texture: wgpu::Texture,
    pub view: wgpu::TextureView,
    pub sampler: wgpu::Sampler,
}

/// Convenient packaging for a device and queue
pub struct GraphicsContext {
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
}

/// For the graph
pub type UID = u32;

pub enum NodeState {
    Loading,
    Ready,
    Broken,
}

// Context traits
// We use finely devided traits to allow extensibility.
// If someone wants to implement a new type of node that requires a new bit of global context,
// they can re-implement "DefaultChain / DefaultContext."

/// This context provides a blank texture
pub trait BlankTextureProvider {
    fn blank_texture(&self) -> Rc<Texture>;
}

/// This context provides a noise (random) texture
pub trait NoiseTextureProvider {
    fn noise_texture(&self) -> Rc<Texture>;
}

/// This context provides a worker pool for running blocking tasks asynchronously
pub trait WorkerPoolProvider {
    type Fut<O: Send + 'static>: Future<Output=O>;
    fn spawn<T: Send + 'static>(&self, f: fn () -> T) -> Self::Fut<T>;
}
