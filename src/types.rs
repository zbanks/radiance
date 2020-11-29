use std::rc::Rc;

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

// Context / chain traits
// We use finely devided traits to allow extensibility.
// If someone wants to implement a new type of node that requires a new bit of global context,
// they can re-implement "DefaultContext."
// If the node instead needs a new bit of per-chain context, they can re-implement
// "DefaultChain"

/// This context / chain provides a blank texture
pub trait BlankTextureProvider {
    fn blank_texture(&self) -> Rc<Texture>;
}

/// This context / chain provides a noise (random) texture
pub trait NoiseTextureProvider {
    fn noise_texture(&self) -> Rc<Texture>;
}
