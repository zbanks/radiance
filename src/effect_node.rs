use crate::types::{Texture, BlankTextureProvider, NoiseTextureProvider, WorkerPoolProvider, WorkHandle, WorkResult, FetchContent};
use std::rc::Rc;
use shaderc;
use std::fmt;

// split into update context & paint context?
pub trait EffectNodeContext = NoiseTextureProvider + BlankTextureProvider + WorkerPoolProvider + FetchContent;

#[derive(Debug)]
pub struct EffectNode<Context: EffectNodeContext> {
    state: EffectNodeState<Context>,
    name: Option<String>,
}

enum EffectNodeState<Context: EffectNodeContext> {
    Uninitialized,
    Compiling {shader_compilation_work_handle: Option<<Context as WorkerPoolProvider>::Handle<Result<Vec<u8>, String>>>},
    Ready {compiled_shader: Vec<u8>},
    Error(String),
}

impl<Context: EffectNodeContext> fmt::Debug for EffectNodeState<Context> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EffectNodeState::Uninitialized => write!(f, "Uninitialized"),
            EffectNodeState::Compiling {shader_compilation_work_handle: _} => write!(f, "Compiling"),
            EffectNodeState::Ready {compiled_shader: _} => write!(f, "Ready"),
            EffectNodeState::Error(e) => write!(f, "Error({})", e),
        }
    }
}

const EFFECT_HEADER: &str = include_str!("effect_header.glsl");

impl<Context: EffectNodeContext> EffectNode<Context> {
    pub fn new() -> EffectNode<Context> {
        EffectNode {
            state: EffectNodeState::Uninitialized,
            name: None,
        }
    }

    fn start_compiling_shader(&mut self, context: &Context) -> EffectNodeState<Context> {

        let shader_content_closure = context.fetch_content_closure(&self.name.as_ref().unwrap());
        let shader_name = self.name.as_ref().unwrap().to_owned();

        let shader_compilation_work_handle = context.spawn(move || {
            let effect_src = shader_content_closure().map_err(|e| e.to_string())?;
            let frag_src = format!("{}{}\n", EFFECT_HEADER, effect_src);
            let mut compiler = shaderc::Compiler::new().unwrap();
            let compilation_result = compiler.compile_into_spirv(&frag_src, shaderc::ShaderKind::Fragment, &shader_name, "main", None);
            match compilation_result {
                Ok(artifact) => Ok(artifact.as_binary_u8().to_vec()),
                Err(e) => Err(e.to_string()),
            }
        });
        EffectNodeState::Compiling {shader_compilation_work_handle: Some(shader_compilation_work_handle)}
    }

    pub fn paint(&mut self, context: &Context, input_textures: Vec<Rc<Texture>>) -> Rc<Texture> {
        // Update state machine

        match &mut self.state {
            EffectNodeState::Uninitialized => {
                match self.name {
                    Some(_) => {self.state = self.start_compiling_shader(context);}
                    None => {},
                };
            },
            EffectNodeState::Compiling {shader_compilation_work_handle: handle_opt} => {
                let handle_ref = handle_opt.as_ref().unwrap();
                if !handle_ref.alive() {
                    let handle = handle_opt.take().unwrap();
                    self.state = match handle.join() {
                        WorkResult::Ok(result) => {
                            match result {
                                Ok(binary) => EffectNodeState::Ready {compiled_shader: binary},
                                Err(msg) => EffectNodeState::Error(msg.to_string()),
                            }
                        },
                        WorkResult::Err(_) => {
                            EffectNodeState::Error("Panicked".to_owned())
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

    pub fn set_name(&mut self, name: &str) {
        self.name = Some(name.to_owned());
    }
}
