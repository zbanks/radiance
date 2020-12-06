use crate::types::{Texture, BlankTexture, NoiseTexture, WorkerPool, WorkHandle, WorkResult, FetchContent, Graphics, Resolution};
use std::rc::Rc;
use shaderc;
use std::fmt;
use wgpu;

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
pub struct EffectNode<UpdateContext: WorkerPool + FetchContent + Graphics> {
    state: EffectNodeState<UpdateContext>,
    name: Option<String>,
}

enum EffectNodeState<UpdateContext: WorkerPool + FetchContent + Graphics> {
    Uninitialized,
    // Note: The work handle below is really not optional.
    // The Option<> is only there to allow "taking" it as soon as compilation is done.
    Compiling {shader_compilation_work_handle: Option<<UpdateContext as WorkerPool>::Handle<Result<Vec<u8>, String>>>},
    Ready {render_pipeline: wgpu::RenderPipeline},
    Error(String),
}

impl<UpdateContext: WorkerPool + FetchContent + Graphics> fmt::Debug for EffectNodeState<UpdateContext> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EffectNodeState::Uninitialized => write!(f, "Uninitialized"),
            EffectNodeState::Compiling {shader_compilation_work_handle: _} => write!(f, "Compiling"),
            EffectNodeState::Ready {render_pipeline: _} => write!(f, "Ready"),
            EffectNodeState::Error(e) => write!(f, "Error({})", e),
        }
    }
}

/// Holds arguments to pass to EffectNode's update() method.
/// This is how you tell the node what it should be doing.
#[derive(Debug)]
pub struct EffectNodeArguments<'a> {
    pub name: Option<&'a str>,
}

/// Return value for EffectNode's update() method.
/// This is how the node tells you about itself.
#[derive(Debug)]
pub struct EffectNodeReturn<'a> {
    pub name: Option<&'a str>,
}

const EFFECT_HEADER: &str = include_str!("effect_header.glsl");

impl<UpdateContext: WorkerPool + FetchContent + Graphics> EffectNode<UpdateContext> {
    pub fn new() -> EffectNode<UpdateContext> {
        EffectNode {
            state: EffectNodeState::Uninitialized,
            name: None,
        }
    }

    // Called when the name changes. Sets the state to Compiling and kicks off shaderc in a worker.
    fn start_compiling_shader(&mut self, context: &UpdateContext) {
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
        self.state = EffectNodeState::Compiling {shader_compilation_work_handle: Some(shader_compilation_work_handle)};
    }

    // Called when the shader compilation is finished. Sets up the render pipeline that will be used in paint calls, and sets the state to Ready.
    fn setup_render_pipeline(&mut self, context: &UpdateContext, frag_binary: &[u8]) {
        let device = &context.graphics_device();

        let vs_module = device.create_shader_module(wgpu::include_spirv!(concat!(env!("OUT_DIR"), "/effect_vertex.spv")));
        let fs_module = device.create_shader_module(wgpu::util::make_spirv(frag_binary));

        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[],
                push_constant_ranges: &[],
            });

        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Render Pipeline"),
            layout: Some(&render_pipeline_layout),
            vertex_stage: wgpu::ProgrammableStageDescriptor {
                module: &vs_module,
                entry_point: "main",
            },
            fragment_stage: Some(wgpu::ProgrammableStageDescriptor {
                module: &fs_module,
                entry_point: "main",
            }),
            rasterization_state: Some(
                wgpu::RasterizationStateDescriptor {
                    front_face: wgpu::FrontFace::Cw,
                    cull_mode: wgpu::CullMode::Back,
                    depth_bias: 0,
                    depth_bias_slope_scale: 0.0,
                    depth_bias_clamp: 0.0,
                    clamp_depth: false,
                }
            ), 
            color_states: &[
                wgpu::ColorStateDescriptor {
                    format: wgpu::TextureFormat::Rgba8UnormSrgb,
                    color_blend: wgpu::BlendDescriptor::REPLACE,
                    alpha_blend: wgpu::BlendDescriptor::REPLACE,
                    write_mask: wgpu::ColorWrite::ALL,
                },
            ],
            primitive_topology: wgpu::PrimitiveTopology::TriangleStrip,
            depth_stencil_state: None,
            vertex_state: wgpu::VertexStateDescriptor {
                index_format: wgpu::IndexFormat::Uint16,
                vertex_buffers: &[],
            },
            sample_count: 1,
            sample_mask: !0,
            alpha_to_coverage_enabled: false,
        });

        self.state = EffectNodeState::Ready {
            render_pipeline,
        };
    }

    pub fn update(&mut self, context: &UpdateContext, args: &EffectNodeArguments) {
        // Update internal state based on args
        // This tree seems pretty big and convoluted just to compare Option<String> with Option<&str>
        let name_changed = match &mut self.name {
            Some(cur_name) => match args.name {
                Some(new_name) => { // Some, Some
                    if cur_name != new_name {
                        self.name = Some(new_name.to_owned());
                        true
                    } else {
                        false
                    }
                },
                None => { // Some, None
                    self.name = None;
                    true
                }
            }
            None => match args.name {
                Some(new_name) => {
                    self.name = Some(new_name.to_owned());
                    true
                },
                None => false, // None, None
            }
        };

        if name_changed {
            // Always recompile if name changed
            match self.name {
                Some(_) => {self.start_compiling_shader(context);}
                None => {self.state = EffectNodeState::Uninitialized;},
            };
        } else if let EffectNodeState::Compiling {shader_compilation_work_handle: handle_opt} = &mut self.state {
            // See if compilation is finished
            let handle_ref = handle_opt.as_ref().unwrap();
            if !handle_ref.alive() {
                let handle = handle_opt.take().unwrap();
                match handle.join() {
                    WorkResult::Ok(result) => {
                        match result {
                            Ok(binary) => {
                                self.setup_render_pipeline(context, &binary);
                            },
                            Err(msg) => {self.state = EffectNodeState::Error(msg.to_string())},
                        }
                    },
                    WorkResult::Err(_) => {
                        self.state = EffectNodeState::Error("Shader compilation panicked".to_owned());
                    },
                }
            }
        }
    }

    /// Call this when a new chain is added to get a PaintState
    /// suitable for use with paint().
    pub fn new_paint_state<PaintContext: Graphics + BlankTexture + NoiseTexture + Resolution>(&self, context: &PaintContext) -> EffectNodePaintState {
        let device = &context.graphics_device();

        let (width, height) = context.resolution();

        let texture_desc = wgpu::TextureDescriptor {
            size: wgpu::Extent3d {
                width,
                height,
                depth: 1,
            },
            //array_layer_count: 1,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsage::COPY_SRC
                | wgpu::TextureUsage::OUTPUT_ATTACHMENT
                ,
            label: None,
        };

        let texture = device.create_texture(&texture_desc);
        let view = texture.create_view(&Default::default());
        let sampler = device.create_sampler(
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

        EffectNodePaintState{
            input_textures: Vec::new(),
            output_texture: Rc::new(Texture {
                texture,
                view,
                sampler,
            }),
        }
    }

    /// Updates the given PaintState.
    /// Paint should be lightweight and not kick off any CPU work (update should do that.)
    pub fn paint<PaintContext: Graphics + BlankTexture + NoiseTexture>(&self, context: &PaintContext, paint_state: &mut EffectNodePaintState) -> (Vec<wgpu::CommandBuffer>, Rc<Texture>) {
        match &self.state {
            EffectNodeState::Ready {render_pipeline} => {

            let mut encoder = context.graphics_device().create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Render Encoder"),
            });

            {
                let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                    color_attachments: &[
                            wgpu::RenderPassColorAttachmentDescriptor {
                                attachment: &paint_state.output_texture.view,
                                resolve_target: None,
                                ops: wgpu::Operations {
                                    load: wgpu::LoadOp::Clear(
                                        wgpu::Color {
                                            r: 0.1,
                                            g: 0.2,
                                            b: 0.3,
                                            a: 1.0,
                                        }
                                    ),
                                    store: true,
                                }
                            }
                        ],
                        depth_stencil_attachment: None,
                    });

                    render_pass.set_pipeline(&render_pipeline);
                    render_pass.draw(0..4, 0..1);
                }

                (vec![encoder.finish()], paint_state.output_texture.clone())
            },
            _ => (vec![], context.blank_texture()),
        }
    }
}
