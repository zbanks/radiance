use crate::types::{Texture, BlankTexture, NoiseTexture, WorkerPool, WorkHandle, WorkResult, FetchContent};
use std::rc::Rc;
use shaderc;
use std::fmt;

/// The EffectNodePaintState contains chain-specific data.
/// It is constructed by calling new_paint_state() and mutated by paint().
/// The application should construct and hold on to one paint state per render chain.
#[derive(Debug)]
pub struct EffectNodePaintState {
    input_textures: Vec<Rc<Texture>>,
    output_texture: Rc<Texture>,
}

/// The EffectNode contains context-specific, chain-agnostic data.
/// It is constructed by calling new()
#[derive(Debug)]
pub struct EffectNode<UpdateContext: WorkerPool + FetchContent> {
    state: EffectNodeState<UpdateContext>,
    name: Option<String>,
}

enum EffectNodeState<UpdateContext: WorkerPool + FetchContent> {
    Uninitialized,
    Compiling {shader_compilation_work_handle: Option<<UpdateContext as WorkerPool>::Handle<Result<Vec<u8>, String>>>},
    Ready {compiled_shader: Vec<u8>},
    Error(String),
}

impl<UpdateContext: WorkerPool + FetchContent> fmt::Debug for EffectNodeState<UpdateContext> {
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

impl<UpdateContext: WorkerPool + FetchContent> EffectNode<UpdateContext> {
    pub fn new() -> EffectNode<UpdateContext> {
        EffectNode {
            state: EffectNodeState::Uninitialized,
            name: None,
        }
    }

    fn start_compiling_shader(&mut self, context: &UpdateContext) -> EffectNodeState<UpdateContext> {

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

    pub fn update(&mut self, context: &UpdateContext) {
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
    }

    /// Call this when a new chain is added to get a PaintState
    /// suitable for use with paint().
    pub fn new_paint_state<PaintContext: BlankTexture + NoiseTexture>(&self, context: &PaintContext) -> EffectNodePaintState {
        EffectNodePaintState {
            input_textures: Vec::new(),
            output_texture: context.blank_texture(),
        }
    }

    /// Updates the given PaintState.
    /// Paint should be lightweight and not kick off any work (update should do that.)
    pub fn paint<PaintContext: BlankTexture + NoiseTexture>(&self, context: &PaintContext, paint_state: &mut EffectNodePaintState) {
        paint_state.output_texture = match self.state {
            EffectNodeState::Ready {compiled_shader: _} => context.blank_texture(), // XXX
            _ => context.blank_texture(),
        };
    }
}
