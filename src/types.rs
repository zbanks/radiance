use std::rc::Rc;

/// Convenient packaging for a texture, view, and sampler
#[derive(Debug)]
pub struct Texture {
    pub texture: wgpu::Texture,
    pub view: wgpu::TextureView,
    pub sampler: wgpu::Sampler,
}

/// Convenient packaging for a device and queue
#[derive(Debug)]
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

pub type WorkResult<T> = Result<T, ()>;

pub trait WorkHandle {
    type Output: Send + 'static;

    fn alive(&self) -> bool;
    fn join(self) -> WorkResult<Self::Output>;
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
    type Handle<T: Send + 'static>: WorkHandle<Output=T>;
    fn spawn<T: Send + 'static>(&self, f: fn () -> T) -> Self::Handle<T>;
}

/// This context provides a graphical context via WGPU
pub trait GraphicsProvider {
    fn graphics(&self) -> Rc<GraphicsContext>;
}
