use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider, WorkerPoolProvider, WorkHandle, WorkResult};
use std::rc::Rc;

pub trait EffectNodeContext: NoiseTextureProvider + BlankTextureProvider + WorkerPoolProvider {}

pub struct EffectNode<Context: EffectNodeContext> {
    shader_compilation_work_handle: Option<<Context as WorkerPoolProvider>::Handle<()>>,
}

impl<Context: EffectNodeContext> EffectNode<Context> {
    pub fn new() -> EffectNode<Context> {
        EffectNode {
            shader_compilation_work_handle: None,
        }
    }

    pub fn paint(&mut self, context: &Context, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        // XXX temp behavior

        match &self.shader_compilation_work_handle {
            None => {
                println!("Spawning");
                self.shader_compilation_work_handle = Some(context.spawn(|| {
                    std::thread::sleep(std::time::Duration::from_millis(3000));
                }));
            },
            Some(h) => {
                if !h.alive() {
                    let h = std::mem::replace(&mut self.shader_compilation_work_handle, None);
                    if let Some(h) = h {
                        match h.join() {
                            WorkResult::Ok(_) => {
                                println!("Finished");
                            },
                            WorkResult::Err(_) => {
                                println!("Panicked!");
                            },
                        }
                    } else {
                        panic!();
                    }
                }
            }
        }

        context.blank_texture()
    }
}
