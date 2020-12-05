use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider, WorkerPoolProvider, WorkHandle, WorkResult};
use std::rc::Rc;
use shaderc;
use std::fmt;

// split into update context & paint context?
pub trait EffectNodeContext = NoiseTextureProvider + BlankTextureProvider + WorkerPoolProvider;

#[derive(Debug)]
pub struct EffectNode<Context: EffectNodeContext> {
    state: EffectNodeState<Context>
}

enum EffectNodeState<Context: EffectNodeContext> {
    Uninitialized,
    Compiling {shader_compilation_work_handle: Option<<Context as WorkerPoolProvider>::Handle<Result<Vec<u8>, shaderc::Error>>>},
    Ready {compiled_shader: Vec<u8>},
    Error {message: String},
}

impl<Context: EffectNodeContext> fmt::Debug for EffectNodeState<Context> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EffectNodeState::Uninitialized => write!(f, "Uninitialized"),
            EffectNodeState::Compiling {shader_compilation_work_handle: _} => write!(f, "Compiling"),
            EffectNodeState::Ready {compiled_shader: _} => write!(f, "Ready"),
            EffectNodeState::Error {message} => write!(f, "Error({:?})", message),
        }
    }
}

impl<Context: EffectNodeContext> EffectNode<Context> {
    pub fn new() -> EffectNode<Context> {
        EffectNode {
            state: EffectNodeState::Uninitialized,
        }
    }

    fn start_compiling_shader(&mut self, context: &Context) -> EffectNodeState<Context> {
        let shader_compilation_work_handle = context.spawn(|| {
            let frag_src = ""; // XXX
            let mut compiler = shaderc::Compiler::new().unwrap();
            let compilation_result = compiler.compile_into_spirv(frag_src, shaderc::ShaderKind::Fragment, "filename.glsl", "main", None);
            match compilation_result {
                Ok(artifact) => Ok(artifact.as_binary_u8().to_vec()),
                Err(e) => Err(e),
            }
        });
        EffectNodeState::Compiling {shader_compilation_work_handle: Some(shader_compilation_work_handle)}
    }

    pub fn paint(&mut self, context: &Context, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        // Update state machine

        match &mut self.state {
            EffectNodeState::Uninitialized => {
                self.state = self.start_compiling_shader(context);
            },
            EffectNodeState::Compiling {shader_compilation_work_handle: handle_opt} => {
                let handle_ref = handle_opt.as_ref().unwrap();
                if !handle_ref.alive() {
                    let handle = handle_opt.take().unwrap();
                    self.state = match handle.join() {
                        WorkResult::Ok(result) => {
                            match result {
                                Ok(binary) => EffectNodeState::Ready {compiled_shader: binary},
                                Err(msg) => EffectNodeState::Error {message: msg.to_string()},
                            }
                        },
                        WorkResult::Err(_) => {
                            EffectNodeState::Error {message: "Panicked".to_string()}
                        },
                    };
                }
            },
            _ => {},
        };

        // Paint
        match self.state {
            EffectNodeState::Ready {compiled_shader: _} => context.blank_texture(), // XXX
            _ => context.blank_texture(),
        }
    }
}
