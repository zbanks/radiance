use crate::context::{ArcTextureViewSampler, Context, Fit, RenderTargetState};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use libmpv::{
    events::{Event, PropertyData},
    render::{OpenGLInitParams, RenderContext, RenderParam, RenderParamApiType},
    FileState, Format, Mpv,
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::ffi;
use std::mem::{transmute, transmute_copy};
use std::string::String;
use std::sync::mpsc;
use std::sync::Arc;
use std::thread;

const SHADER_SOURCE: &str = include_str!("image_shader.wgsl");

fn get_proc_address(
    ctx: &(Arc<surfman::Device>, Arc<surfman::Context>),
    name: &str,
) -> *mut ffi::c_void {
    let (device, context) = ctx;
    device.get_proc_address(&context, name) as *mut ffi::c_void
}

/// Properties of an MovieNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct MovieNodeProps {
    pub name: String,
    pub mute: Option<bool>,
    pub pause: Option<bool>,
    pub position: Option<f64>,
    pub fit: Option<Fit>,
}

impl From<&MovieNodeProps> for CommonNodeProps {
    fn from(_props: &MovieNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

#[allow(clippy::large_enum_variant)]
pub enum MovieNodeState {
    Uninitialized,
    Ready(MovieNodeStateReady),
    Error_(String), // ambiguous_associated_items error triggered by derive_more::TryInto without the _
}

#[derive(Debug)]
enum MpvThreadEvent {
    RedrawRequested,
    Resize(u32, u32),
    Terminate,
    Duration(f64),
    Position(f64),
    Playing,
    Error(String),
    Mute(bool),
    Pause(bool),
    Seek(f64),
}

#[derive(Debug)]
enum MpvThreadStatusUpdate {
    Texture(ArcTextureViewSampler, u32, u32),
    Duration(f64),
    Position(f64),
    Playing,
    Error(String),
}

#[derive(Debug)]
pub struct MovieNodeStateReady {
    // Cached props
    name: String,
    mute: bool,
    pause: bool,
    fit: Fit,

    // GPU resources
    frame_texture: ArcTextureViewSampler,
    frame_width: u32,
    frame_height: u32,
    bind_group_layout: wgpu::BindGroupLayout,
    uniform_buffer: wgpu::Buffer,
    sampler: wgpu::Sampler,
    render_pipeline: wgpu::RenderPipeline,

    // Paint states
    paint_states: HashMap<RenderTargetId, MovieNodePaintState>,

    // MPV thread
    _mpv_thread: Option<thread::JoinHandle<()>>,
    mpv_tx: mpsc::Sender<MpvThreadEvent>,
    mpv_rx: mpsc::Receiver<MpvThreadStatusUpdate>,

    // Status
    pub playing: bool,
    pub duration: f64,
    pub position: f64,
}

impl Drop for MovieNodeStateReady {
    fn drop(&mut self) {
        let _ = self.mpv_tx.send(MpvThreadEvent::Terminate);
        //self._mpv_thread
        //    .take()
        //    .map(|mpv_thread| mpv_thread.join().unwrap());
    }
}

#[derive(Debug)]
struct MovieNodePaintState {
    output_texture: ArcTextureViewSampler,
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    factor: [f32; 2],
    _padding: [u8; 4],
}

// This is a state machine, it's more natural to use `match` than `if let`
#[allow(clippy::single_match)]
impl MovieNodeState {
    fn setup_render_pipeline(
        _ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &MovieNodeProps,
    ) -> Result<MovieNodeStateReady, String> {
        let name = &props.name;

        // Temporary 1x1 texture since we don't know the width & height yet
        let initial_frame_texture = {
            let texture_desc = wgpu::TextureDescriptor {
                size: wgpu::Extent3d {
                    width: 1,
                    height: 1,
                    depth_or_array_layers: 1,
                },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8UnormSrgb,
                usage: wgpu::TextureUsages::COPY_DST | wgpu::TextureUsages::TEXTURE_BINDING,
                label: Some("image"),
                view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
            };
            let texture = device.create_texture(&texture_desc);
            let view = texture.create_view(&Default::default());
            let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            });
            ArcTextureViewSampler::new(texture, view, sampler)
        };

        // Spin up MPV thread

        let (mpv_thread, mpv_tx, mpv_rx) = {
            // Variables that will be moved into thread:
            let (event_tx, event_rx) = mpsc::channel();
            let return_tx = event_tx.clone();
            let (status_tx, status_rx) = mpsc::channel();
            let name = name.clone();
            let queue = queue.clone();
            let device = device.clone();
            (
                thread::spawn(move || {
                    struct FrameResources {
                        width: u32,
                        height: u32,
                        wgpu_texture: ArcTextureViewSampler,
                        fbo: glow::NativeFramebuffer,
                        ogl_texture: glow::NativeTexture,
                        pixels: Vec<u8>,
                    }

                    // Initialize OpenGL
                    let gl_connection = surfman::Connection::new().unwrap();
                    let gl_adapter = gl_connection.create_adapter().unwrap();
                    let mut gl_device = gl_connection.create_device(&gl_adapter).unwrap();
                    let gl_context_attributes = surfman::ContextAttributes {
                        version: surfman::GLVersion::new(3, 3),
                        flags: surfman::ContextAttributeFlags::empty(),
                    };

                    let gl_context_descriptor = gl_device
                        .create_context_descriptor(&gl_context_attributes)
                        .unwrap();
                    let gl_context = gl_device
                        .create_context(&gl_context_descriptor, None)
                        .unwrap();
                    let gl_device = Arc::new(gl_device);
                    let gl_context = Arc::new(gl_context);

                    gl_device.make_context_current(&gl_context).unwrap();

                    let gl = unsafe {
                        glow::Context::from_loader_function(|s| {
                            gl_device.get_proc_address(&gl_context, s)
                        })
                    };
                    use glow::HasContext;

                    let mut frame_resources: Option<FrameResources> = None;

                    // Initialize MPV
                    let mut mpv = Mpv::with_initializer(|mpv_init| {
                        {
                            let opt = ffi::CString::new("terminal").unwrap();
                            let val = ffi::CString::new("yes").unwrap();
                            unsafe {
                                libmpv_sys::mpv_set_option_string(
                                    transmute_copy(&mpv_init),
                                    opt.as_ptr(),
                                    val.as_ptr(),
                                );
                            }
                        }
                        {
                            let opt = ffi::CString::new("msg-level").unwrap();
                            let val = ffi::CString::new("all=v").unwrap();
                            unsafe {
                                libmpv_sys::mpv_set_option_string(
                                    transmute_copy(&mpv_init),
                                    opt.as_ptr(),
                                    val.as_ptr(),
                                );
                            }
                        }
                        {
                            let opt = ffi::CString::new("ytdl").unwrap();
                            let val = ffi::CString::new("yes").unwrap();
                            unsafe {
                                libmpv_sys::mpv_set_option_string(
                                    transmute_copy(&mpv_init),
                                    opt.as_ptr(),
                                    val.as_ptr(),
                                );
                            }
                        }
                        {
                            let opt = ffi::CString::new("osd-level").unwrap();
                            let val = ffi::CString::new("0").unwrap();
                            unsafe {
                                libmpv_sys::mpv_set_option_string(
                                    transmute_copy(&mpv_init),
                                    opt.as_ptr(),
                                    val.as_ptr(),
                                );
                            }
                        }
                        Ok(())
                    })
                    .unwrap();

                    let create_render_context_args = vec![
                        RenderParam::ApiType(RenderParamApiType::OpenGl),
                        RenderParam::InitParams(OpenGLInitParams {
                            get_proc_address,
                            ctx: (gl_device.clone(), gl_context.clone()),
                        }),
                    ];

                    let mut render_context =
                        RenderContext::new(unsafe { mpv.ctx.as_mut() }, create_render_context_args)
                            .expect("Failed creating render context");

                    let render_tx = event_tx.clone();
                    render_context.set_update_callback(move || {
                        render_tx.send(MpvThreadEvent::RedrawRequested).unwrap();
                    });

                    mpv.set_property("loop", "inf").unwrap();
                    mpv.set_property("mute", true).unwrap();

                    let mut ev_ctx = mpv.create_event_context();
                    ev_ctx.disable_deprecated_events().unwrap();
                    ev_ctx
                        .observe_property("duration", Format::Double, 0)
                        .unwrap();
                    ev_ctx
                        .observe_property("time-pos", Format::Double, 0)
                        .unwrap();
                    ev_ctx
                        .observe_property("video-params/w", Format::Int64, 0)
                        .unwrap();
                    ev_ctx
                        .observe_property("video-params/h", Format::Int64, 0)
                        .unwrap();

                    mpv.playlist_load_files(&[(
                        &format!("library/{}", name),
                        FileState::AppendPlay,
                        None,
                    )])
                    .unwrap();

                    // The wakeup callback doesn't seem to work right, so we spawn a new thread
                    crossbeam::thread::scope(|s| {
                        s.spawn(move |_| {
                            let mut w: Option<u32> = None;
                            let mut h: Option<u32> = None;
                            loop {
                                let ev = ev_ctx.wait_event(10.);
                                if ev.is_none() { continue; }
                                match ev.unwrap() {
                                    Ok(Event::EndFile(r)) => {
                                        eprintln!("MPV Exiting. Reason: {:?}", r);
                                        event_tx.send(MpvThreadEvent::Terminate).unwrap();
                                        break;
                                    }
                                    Ok(Event::PropertyChange {
                                        name: "duration",
                                        change: PropertyData::Double(d),
                                        ..
                                    }) => {
                                        event_tx.send(MpvThreadEvent::Duration(d)).unwrap();
                                    }
                                    Ok(Event::PropertyChange {
                                        name: "time-pos",
                                        change: PropertyData::Double(p),
                                        ..
                                    }) => {
                                        event_tx.send(MpvThreadEvent::Position(p)).unwrap();
                                    }
                                    Ok(Event::PropertyChange {
                                        name: "video-params/w",
                                        change: PropertyData::Int64(new_w),
                                        ..
                                    }) => {
                                        w = Some(new_w as u32);
                                        if let (Some(have_w), Some(have_h)) = (w, h) {
                                            event_tx.send(MpvThreadEvent::Resize(have_w, have_h)).unwrap();
                                        }
                                    }
                                    Ok(Event::PropertyChange {
                                        name: "video-params/h",
                                        change: PropertyData::Int64(new_h),
                                        ..
                                    }) => {
                                        h = Some(new_h as u32);
                                        if let (Some(have_w), Some(have_h)) = (w, h) {
                                            event_tx.send(MpvThreadEvent::Resize(have_w, have_h)).unwrap();
                                        }
                                    }
                                    Ok(Event::PlaybackRestart) => {
                                            event_tx.send(MpvThreadEvent::Playing).unwrap();
                                    }
                                    Ok(e) => eprintln!("Event triggered: {:?}", e),
                                    Err(e) => {
                                        eprintln!("MPV Error: {:?}", e);
                                        event_tx.send(MpvThreadEvent::Error(String::from("MPV Error"))).unwrap();
                                        event_tx.send(MpvThreadEvent::Terminate).unwrap();
                                        break;
                                    }
                                }
                            }
                        });

                        loop {
                            let ev = event_rx.recv().unwrap();

                            match ev {
                                MpvThreadEvent::Resize(width, height) => {
                                    let wgpu_texture = {
                                        let texture_desc = wgpu::TextureDescriptor {
                                            size: wgpu::Extent3d {
                                                width,
                                                height,
                                                depth_or_array_layers: 1,
                                            },
                                            mip_level_count: 1,
                                            sample_count: 1,
                                            dimension: wgpu::TextureDimension::D2,
                                            format: wgpu::TextureFormat::Rgba8UnormSrgb,
                                            usage: wgpu::TextureUsages::COPY_DST | wgpu::TextureUsages::TEXTURE_BINDING,
                                            label: Some("image"),
                                            view_formats: &[wgpu::TextureFormat::Rgba8UnormSrgb],
                                        };
                                        let texture = device.create_texture(&texture_desc);
                                        let view = texture.create_view(&Default::default());
                                        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
                                            address_mode_u: wgpu::AddressMode::ClampToEdge,
                                            address_mode_v: wgpu::AddressMode::ClampToEdge,
                                            address_mode_w: wgpu::AddressMode::ClampToEdge,
                                            mag_filter: wgpu::FilterMode::Linear,
                                            min_filter: wgpu::FilterMode::Linear,
                                            mipmap_filter: wgpu::FilterMode::Linear,
                                            ..Default::default()
                                        });
                                        ArcTextureViewSampler::new(texture, view, sampler)
                                    };

                                    let (fbo, ogl_texture) = unsafe {
                                        let fbo = gl.create_framebuffer().unwrap();
                                        gl.bind_framebuffer(glow::FRAMEBUFFER, Some(fbo));

                                        let texture = gl.create_texture().unwrap();
                                        gl.bind_texture(glow::TEXTURE_2D, Some(texture));
                                        gl.tex_image_2d(
                                            glow::TEXTURE_2D,
                                            0,
                                            glow::RGBA as i32,
                                            width as i32,
                                            height as i32,
                                            0,
                                            glow::RGBA,
                                            glow::UNSIGNED_BYTE,
                                            None,
                                        );
                                        gl.tex_parameter_i32(
                                            glow::TEXTURE_2D,
                                            glow::TEXTURE_MIN_FILTER,
                                            glow::LINEAR as i32,
                                        );
                                        gl.tex_parameter_i32(
                                            glow::TEXTURE_2D,
                                            glow::TEXTURE_MAG_FILTER,
                                            glow::LINEAR as i32,
                                        );

                                        gl.framebuffer_texture_2d(
                                            glow::FRAMEBUFFER,
                                            glow::COLOR_ATTACHMENT0,
                                            glow::TEXTURE_2D,
                                            Some(texture),
                                            0,
                                        );

                                        assert_eq!(
                                            gl.check_framebuffer_status(glow::FRAMEBUFFER),
                                            glow::FRAMEBUFFER_COMPLETE
                                        );

                                        gl.bind_framebuffer(glow::FRAMEBUFFER, None);
                                        gl.bind_texture(glow::TEXTURE_2D, None);

                                        (fbo, texture)
                                    };

                                    let pixels = vec![0; (width * height * 4) as usize];

                                    status_tx.send(MpvThreadStatusUpdate::Texture(wgpu_texture.clone(), width, height)).unwrap();
                                    frame_resources = Some(FrameResources {
                                        width,
                                        height,
                                        wgpu_texture,
                                        fbo,
                                        ogl_texture,
                                        pixels,
                                    });
                                }
                                MpvThreadEvent::RedrawRequested => {
                                    if let Some(frame_resources) = frame_resources.as_mut() {
                                        // Tell MPV to render into our FBO
                                        render_context
                                            .render::<(Arc<surfman::Device>, Arc<surfman::Context>)>(
                                                unsafe { transmute(frame_resources.fbo) },
                                                frame_resources.width as i32,
                                                frame_resources.height as i32,
                                                true,
                                            )
                                            .expect("Failed to render MPV frame into framebuffer");

                                        // TODO: Replace this with a memory copy that never leaves the GPU
                                        // (this approach is SLOW!)

                                        // Read pixels out of the corresponding OpenGL texture
                                        unsafe {
                                            gl.bind_texture(glow::TEXTURE_2D, Some(frame_resources.ogl_texture));
                                            gl.get_tex_image(
                                                glow::TEXTURE_2D,
                                                0,
                                                glow::RGBA,
                                                glow::UNSIGNED_BYTE,
                                                glow::PixelPackData::Slice(&mut frame_resources.pixels),
                                            );
                                            gl.bind_texture(glow::TEXTURE_2D, None);
                                        }

                                        // Write pixels into WGPU texture
                                        let frame_size = wgpu::Extent3d {
                                            width: frame_resources.width,
                                            height: frame_resources.height,
                                            depth_or_array_layers: 1,
                                        };
                                        queue.write_texture(
                                            wgpu::TexelCopyTextureInfo {
                                                texture: &frame_resources.wgpu_texture.texture,
                                                mip_level: 0,
                                                origin: wgpu::Origin3d::ZERO,
                                                aspect: wgpu::TextureAspect::All,
                                            },
                                            &frame_resources.pixels,
                                            wgpu::TexelCopyBufferLayout {
                                                offset: 0,
                                                bytes_per_row: Some(
                                                    4 * frame_resources.width,
                                                ),
                                                rows_per_image: Some(
                                                    frame_resources.height,
                                                ),
                                            },
                                            frame_size,
                                        );
                                    }
                                }
                                MpvThreadEvent::Terminate => {
                                    break;
                                }
                                MpvThreadEvent::Duration(d) => {
                                    status_tx.send(MpvThreadStatusUpdate::Duration(d)).unwrap();
                                }
                                MpvThreadEvent::Position(p) => {
                                    status_tx.send(MpvThreadStatusUpdate::Position(p)).unwrap();
                                }
                                MpvThreadEvent::Playing => {
                                    status_tx.send(MpvThreadStatusUpdate::Playing).unwrap();
                                }
                                MpvThreadEvent::Error(e) => {
                                    status_tx.send(MpvThreadStatusUpdate::Error(e)).unwrap();
                                }
                                MpvThreadEvent::Mute(mute) => {
                                    mpv.set_property("mute", mute).unwrap();
                                }
                                MpvThreadEvent::Pause(pause) => {
                                    mpv.set_property("pause", pause).unwrap();
                                }
                                MpvThreadEvent::Seek(position) => {
                                    let position_str = format!("{}", position);
                                    mpv.command("seek", &[&position_str, "absolute"]).unwrap();
                                }
                            }
                        }

                        mpv.command("quit", &[]).unwrap();
                    }).unwrap();
                }),
                return_tx,
                status_rx,
            )
        };

        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some(&format!("MovieNode {}", name)),
            source: wgpu::ShaderSource::Wgsl(SHADER_SOURCE.into()),
        });

        // The uniforms bind group:
        // 0: Uniforms
        // 1: iSampler
        // 2: iInputTex
        // 3: iImageTex

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0, // Uniforms
                    visibility: wgpu::ShaderStages::VERTEX,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1, // iSampler
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2, // iInputTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 3, // iImageTex
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        multisampled: false,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    },
                    count: None,
                },
            ],
            label: Some(&format!("MovieNode {} bind group layout", name)),
        });

        let render_pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Render Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        // Create a render pipeline
        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some(&format!("MovieNode {} render pipeline", name)),
            layout: Some(&render_pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: Some("vs_main"),
                buffers: &[],
                compilation_options: Default::default(),
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: Some("fs_main"),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: Default::default(),
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleStrip,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: Some(wgpu::Face::Back),
                polygon_mode: wgpu::PolygonMode::Fill,
                unclipped_depth: false,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
            cache: None,
        });

        // The update uniform buffer for this effect
        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some(&format!("MovieNode {} uniform buffer", name)),
            size: std::mem::size_of::<Uniforms>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        // The sampler that will be used for texture access within the shaders
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        Ok(MovieNodeStateReady {
            name: name.clone(),
            mute: true,
            pause: false,
            fit: Fit::Crop,
            frame_texture: initial_frame_texture,
            frame_width: 1,
            frame_height: 1,
            bind_group_layout,
            uniform_buffer,
            sampler,
            render_pipeline,
            paint_states: HashMap::new(),
            _mpv_thread: Some(mpv_thread),
            mpv_tx,
            mpv_rx,
            playing: false,
            duration: 0.,
            position: 0.,
        })
    }

    fn new_paint_state(
        _self_ready: &MovieNodeStateReady,
        _ctx: &Context,
        device: &wgpu::Device,
        _queue: &wgpu::Queue,
        render_target_state: &RenderTargetState,
    ) -> MovieNodePaintState {
        let texture_desc = wgpu::TextureDescriptor {
            size: wgpu::Extent3d {
                width: render_target_state.width(),
                height: render_target_state.height(),
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::COPY_SRC
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::TEXTURE_BINDING,
            label: None,
            view_formats: &[wgpu::TextureFormat::Rgba8Unorm],
        };

        let make_texture = || {
            let texture = device.create_texture(&texture_desc);
            let view = texture.create_view(&Default::default());
            let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
                address_mode_u: wgpu::AddressMode::ClampToEdge,
                address_mode_v: wgpu::AddressMode::ClampToEdge,
                address_mode_w: wgpu::AddressMode::ClampToEdge,
                mag_filter: wgpu::FilterMode::Linear,
                min_filter: wgpu::FilterMode::Linear,
                mipmap_filter: wgpu::FilterMode::Linear,
                ..Default::default()
            });
            ArcTextureViewSampler::new(texture, view, sampler)
        };

        let output_texture = make_texture();

        MovieNodePaintState { output_texture }
    }

    fn update_paint_states(
        self_ready: &mut MovieNodeStateReady,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
    ) {
        // See if we need to add or remove any paint states
        // (based on the context's render targets)

        self_ready
            .paint_states
            .retain(|id, _| ctx.render_target_states().contains_key(id));

        for (check_render_target_id, render_target_state) in ctx.render_target_states().iter() {
            if !self_ready.paint_states.contains_key(check_render_target_id) {
                self_ready.paint_states.insert(
                    *check_render_target_id,
                    Self::new_paint_state(self_ready, ctx, device, queue, render_target_state),
                );
            }
        }
    }

    pub fn new(
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &MovieNodeProps,
    ) -> Self {
        // TODO kick of shader compilation in the background instead of blocking
        match Self::setup_render_pipeline(ctx, device, queue, props) {
            Ok(mut new_obj_ready) => {
                Self::update_paint_states(&mut new_obj_ready, ctx, device, queue);
                Self::Ready(new_obj_ready)
            }
            Err(msg) => {
                eprintln!("Unable to configure MovieNode: {}", msg);
                Self::Error_(msg)
            }
        }
    }

    pub fn update(
        &mut self,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        props: &mut MovieNodeProps,
    ) {
        let mut set_error: Option<String> = None;
        match self {
            MovieNodeState::Ready(self_ready) => {
                if props.name != self_ready.name {
                    *self = MovieNodeState::Error_(
                        "MovieNode name changed after construction".to_string(),
                    );
                    return;
                }
                if self_ready.playing {
                    match props.mute {
                        Some(mute) => {
                            if self_ready.mute != mute {
                                let _ = self_ready.mpv_tx.send(MpvThreadEvent::Mute(mute));
                                self_ready.mute = mute;
                            }
                        }
                        _ => {}
                    }
                    match props.pause {
                        Some(pause) => {
                            if self_ready.pause != pause {
                                let _ = self_ready.mpv_tx.send(MpvThreadEvent::Pause(pause));
                                self_ready.pause = pause;
                            }
                        }
                        _ => {}
                    }
                    match props.position {
                        Some(position) => {
                            if self_ready.position != position {
                                let _ = self_ready.mpv_tx.send(MpvThreadEvent::Seek(position));
                                self_ready.position = position;
                            }
                        }
                        _ => {}
                    }
                }
                match &props.fit {
                    Some(fit) => {
                        self_ready.fit = fit.clone();
                    }
                    _ => {}
                }

                // Process any MPV status messages we have recieved
                while let Ok(mpv_status) = self_ready.mpv_rx.try_recv() {
                    match mpv_status {
                        MpvThreadStatusUpdate::Texture(t, width, height) => {
                            self_ready.frame_texture = t;
                            self_ready.frame_width = width;
                            self_ready.frame_height = height;
                        }
                        MpvThreadStatusUpdate::Duration(d) => {
                            self_ready.duration = d;
                        }
                        MpvThreadStatusUpdate::Position(p) => {
                            self_ready.position = p;
                        }
                        MpvThreadStatusUpdate::Playing => {
                            self_ready.playing = true;
                        }
                        MpvThreadStatusUpdate::Error(err) => {
                            set_error = Some(err);
                        }
                    }
                }

                // Report back to the caller what our props are
                self_ready.update_props(props);

                Self::update_paint_states(self_ready, ctx, device, queue);
            }
            _ => {}
        }

        if let Some(err) = set_error {
            *self = Self::Error_(err);
        }
    }

    pub fn paint(
        &mut self,
        ctx: &Context,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        encoder: &mut wgpu::CommandEncoder,
        render_target_id: RenderTargetId,
        inputs: &[Option<ArcTextureViewSampler>],
    ) -> ArcTextureViewSampler {
        match self {
            MovieNodeState::Ready(self_ready) => {
                let paint_state = self_ready.paint_states.get_mut(&render_target_id).expect("Call to paint() with a render target ID unknown to the node (did you call update() first?)");

                let render_target_state = ctx
                    .render_target_state(render_target_id)
                    .expect("Call to paint() with a render target ID unknown to the context");

                let media_size = (
                    self_ready.frame_width as f32,
                    self_ready.frame_height as f32,
                );
                let canvas_size = (
                    render_target_state.width() as f32,
                    render_target_state.height() as f32,
                );
                let (factor_fit_x, factor_fit_y) = self_ready.fit.factor(media_size, canvas_size);
                let factor_fit_y = -factor_fit_y; // MPV writes frames that are vertically mirrored

                // Populate the uniforms
                {
                    let uniforms = Uniforms {
                        factor: [factor_fit_x, factor_fit_y],
                        ..Default::default()
                    };
                    queue.write_buffer(
                        &self_ready.uniform_buffer,
                        0,
                        bytemuck::cast_slice(&[uniforms]),
                    );
                }

                // Make an array of input textures
                let input_texture = if let Some(Some(tex)) = inputs.get(0) {
                    tex
                } else {
                    ctx.blank_texture()
                };

                let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
                    layout: &self_ready.bind_group_layout,
                    entries: &[
                        wgpu::BindGroupEntry {
                            binding: 0,
                            resource: self_ready.uniform_buffer.as_entire_binding(),
                        },
                        wgpu::BindGroupEntry {
                            binding: 1,
                            resource: wgpu::BindingResource::Sampler(&self_ready.sampler),
                        },
                        wgpu::BindGroupEntry {
                            binding: 2, // iInputTex
                            resource: wgpu::BindingResource::TextureView(&input_texture.view),
                        },
                        wgpu::BindGroupEntry {
                            binding: 3, // iImageTex
                            resource: wgpu::BindingResource::TextureView(
                                &self_ready.frame_texture.view,
                            ),
                        },
                    ],
                    label: Some("MovieNode bind group"),
                });

                {
                    let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("MovieNode render pass"),
                        color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                            view: paint_state.output_texture.view.as_ref(),
                            resolve_target: None,
                            ops: wgpu::Operations {
                                load: wgpu::LoadOp::Load,
                                store: wgpu::StoreOp::Store,
                            },
                            depth_slice: None,
                        })],
                        depth_stencil_attachment: None,
                        timestamp_writes: None,
                        occlusion_query_set: None,
                    });

                    render_pass.set_pipeline(&self_ready.render_pipeline);
                    render_pass.set_bind_group(0, &bind_group, &[]);
                    render_pass.draw(0..4, 0..1);
                }

                paint_state.output_texture.clone()
            }

            _ => ctx.blank_texture().clone(),
        }
    }
}

impl MovieNodeStateReady {
    fn update_props(&self, props: &mut MovieNodeProps) {
        props.name.clone_from(&self.name);
        props.mute = Some(self.mute);
        props.pause = Some(self.pause);
        props.position = Some(self.position);
        props.fit = Some(self.fit.clone());
    }
}
