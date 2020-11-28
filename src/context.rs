use crate::chain::Chain;
use wgpu;

pub struct GraphicsContext<'a> {
    device: &'a wgpu::Device,
    queue: &'a wgpu::Queue,
}

pub struct Context<'a> {
    chains: Vec<Chain>,
    graphics_context: GraphicsContext<'a>,
}

impl<'a> Context<'a> {
    pub fn new(device: &'a wgpu::Device, queue: &'a wgpu::Queue) -> Context<'a> {
        Context {
            chains: Vec::new(),
            graphics_context: GraphicsContext {
                device: device,
                queue: queue,
            }
        }
    }
}
