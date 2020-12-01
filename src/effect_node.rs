use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider, WorkerPoolProvider};
//use wgpu;
use std::rc::Rc;
use futures::task::Poll;

//trait ShaderCompilationFuture: Future<Output=()> {}

trait EffectNodeContext: NoiseTextureProvider + BlankTextureProvider + WorkerPoolProvider {}

pub struct EffectNode {
//    shader_compilation_future: Option<SCF>,
}

impl EffectNode {

    pub fn new() -> EffectNode {
        EffectNode {
//            shader_compilation_future: None,
        }
    }

    pub fn paint<Context: EffectNodeContext>(&mut self, context: Rc<Context>, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        // XXX temp behavior
        match self.shader_compilation_future {
            None => {
                println!("Spawning");
                self.shader_compilation_future = context.spawn(|| {
                    std::thread::sleep(std::time::Duration::from_millis(3000));
                });
            },
            Some(f) => {
                match f.poll() {
                    Poll::Ready(t) => {
                        println!("Finished");
                    },
                    Poll::Pending => {
                        println!("Pending");
                    }
                }
            }
        }
        context.blank_texture()
    }
}
