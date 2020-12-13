use radiance::{DefaultContext, BlankTexture, EffectNode, EffectNodeArguments, Texture};
use winit::{
    event::*,
    event_loop::{EventLoop, ControlFlow},
    window::{Window, WindowBuilder},
};
use futures::executor::block_on;
use imgui::*;
use radiance::imgui_wgpu;
use std::time::Duration;

struct State {
    pub surface: wgpu::Surface,
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    pub sc_desc: wgpu::SwapChainDescriptor,
    pub swap_chain: wgpu::SwapChain,
    pub size: winit::dpi::PhysicalSize<u32>,
}

impl State {
    async fn new(window: &Window) -> Self {
        let size = window.inner_size();

        // The instance is a handle to our GPU
        // BackendBit::PRIMARY => Vulkan + Metal + DX12 + Browser WebGPU
        let instance = wgpu::Instance::new(wgpu::BackendBit::PRIMARY);
        let surface = unsafe { instance.create_surface(window) };
        let adapter = instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::Default,
                compatible_surface: Some(&surface),
            })
            .await
            .unwrap();

        let (device, queue) = adapter
            .request_device(
                &wgpu::DeviceDescriptor {
                    features: wgpu::Features::SAMPLED_TEXTURE_BINDING_ARRAY, // Need to remove for web port
                    limits: wgpu::Limits::default(),
                    shader_validation: true,
                },
                None, // Trace path
            )
            .await
            .unwrap();

        let sc_desc = wgpu::SwapChainDescriptor {
            usage: wgpu::TextureUsage::OUTPUT_ATTACHMENT,
            format: wgpu::TextureFormat::Bgra8UnormSrgb,
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::Fifo,
        };
        let swap_chain = device.create_swap_chain(&surface, &sc_desc);

        Self {
            surface,
            device,
            queue,
            sc_desc,
            swap_chain,
            size,
        }
    }

    fn resize(&mut self, new_size: winit::dpi::PhysicalSize<u32>) {
        self.size = new_size;
        self.sc_desc.width = new_size.width;
        self.sc_desc.height = new_size.height;
        self.swap_chain = self.device.create_swap_chain(&self.surface, &self.sc_desc);
    }
}

fn render_imgui(winit_window: &Window, state: &mut State, imgui: &mut imgui::Context, platform: &mut imgui_winit_support::WinitPlatform, renderer: &mut imgui_wgpu::Renderer, purple_tex_id: TextureId) {
    // Update the UI
    platform
        .prepare_frame(imgui.io_mut(), winit_window)
        .expect("Failed to prepare frame!");

    // Build the UI
    let ui = imgui.frame();
    {
        let window = imgui::Window::new(im_str!("Hello Imgui from WGPU!"));
        window
            .size([300.0, 200.0], Condition::FirstUseEver)
            .build(&ui, || {
                ui.text(im_str!("Hello world!"));
                ui.text(im_str!("This is a demo of imgui-rs using imgui-wgpu!"));
                ui.separator();
                let mouse_pos = ui.io().mouse_pos;
                ui.text(im_str!(
                    "Mouse Position: ({:.1}, {:.1})",
                    mouse_pos[0],
                    mouse_pos[1],
                ));
                ui.separator();
                imgui::Image::new(purple_tex_id, [100.0, 100.0]).build(&ui);
            });
    }

    // Prepare to render
    let mut encoder = state.device.create_command_encoder(&Default::default());
    let output = match state.swap_chain.get_current_frame() {
        Ok(frame) => frame,
        Err(e) => {
            eprintln!("Error getting frame: {:?}", e);
            return;
        }
    }
    .output;

    platform.prepare_render(&ui, winit_window);

    let mut pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
        color_attachments: &[wgpu::RenderPassColorAttachmentDescriptor {
            attachment: &output.view,
            resolve_target: None,
            ops: wgpu::Operations {
                load: wgpu::LoadOp::Clear(wgpu::Color {
                    r: 0.,
                    g: 0.,
                    b: 0.,
                    a: 1.,
                }),
                store: true,
            },
        }],
        depth_stencil_attachment: None,
    });
    renderer
        .render(ui.render(), &state.queue, &state.device, &mut pass)
        .expect("Failed to render UI!");
    drop(pass);

    state.queue.submit(Some(encoder.finish()));
}

fn main() {
    // Set up winit
    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .build(&event_loop)
        .unwrap();

    // Set up wgpu
    let mut state: State = block_on(State::new(&window));

    // Set up imgui
    let mut imgui = imgui::Context::create();
    let mut platform = imgui_winit_support::WinitPlatform::init(&mut imgui);
    platform.attach_window(
        imgui.io_mut(), 
        &window,
        imgui_winit_support::HiDpiMode::Default,
    );
    imgui.set_ini_filename(None);

    let hidpi_factor = window.scale_factor();
    let font_size = (13.0 * hidpi_factor) as f32;
    imgui.io_mut().font_global_scale = (1.0 / hidpi_factor) as f32;
    imgui.fonts().add_font(&[FontSource::DefaultFontData {
        config: Some(imgui::FontConfig {
            oversample_h: 1,
            pixel_snap_h: true,
            size_pixels: font_size,
            ..Default::default()
        }),
    }]);

    let renderer_config = imgui_wgpu::RendererConfig {
        texture_format: state.sc_desc.format,
        ..Default::default()
    };
    let mut renderer = imgui_wgpu::Renderer::new(&mut imgui, &state.device, &state.queue, renderer_config);

    // Create a radiance Context
    let mut ctx = DefaultContext::new(&state.device, &state.queue);

    let texture_size = 256;
    let test_chain_id = ctx.add_chain((texture_size, texture_size));
    let mut effect_node = EffectNode::new();

    let chain = ctx.chain(test_chain_id).unwrap();
    let mut paint_state = effect_node.new_paint_state(chain);

    let args = EffectNodeArguments {
        name: Some("purple.glsl"),
    };

    let mut purple_tex = None;

    for i in 0..10 {
        // Update and render effect node
        println!("update...");
        effect_node.update(&ctx, &args);
        println!("{:?}", effect_node);
        println!("paint...");
        let (cmds, tex) = effect_node.paint(chain, &mut paint_state);
        state.queue.submit(cmds);

        std::thread::sleep(Duration::from_millis(10));
        purple_tex = Some(tex);
    }

    let purple_tex_id = renderer.textures.insert(imgui_wgpu::Texture::from_radiance(purple_tex.unwrap().clone(), &state.device, &renderer));

    event_loop.run(move |event, _, control_flow| {
        platform.handle_event(imgui.io_mut(), &window, &event);
        match event {
            Event::WindowEvent {
                ref event,
                window_id,
            } if window_id == window.id() => {
                //if !state.input(event) {
                    match event {
                        WindowEvent::CloseRequested => *control_flow = ControlFlow::Exit,
                        WindowEvent::KeyboardInput { input, .. } => match input {
                            KeyboardInput {
                                state: ElementState::Pressed,
                                virtual_keycode: Some(VirtualKeyCode::Escape),
                                ..
                            } => *control_flow = ControlFlow::Exit,
                            _ => {}
                        },
                        WindowEvent::Resized(physical_size) => {
                            state.resize(*physical_size);
                        }
                        WindowEvent::ScaleFactorChanged { new_inner_size, .. } => {
                            // new_inner_size is &&mut so w have to dereference it twice
                            state.resize(**new_inner_size);
                        }
                        _ => {}
                    }
                //}
            }
            Event::RedrawRequested(_) => {
                render_imgui(&window, &mut state, &mut imgui, &mut platform, &mut renderer, purple_tex_id);
                //state.update();
                //match state.render() {
                //    Ok(_) => {}
                //    // Recreate the swap_chain if lost
                //    Err(wgpu::SwapChainError::Lost) => state.resize(state.size),
                //    // The system is out of memory, we should probably quit
                //    Err(wgpu::SwapChainError::OutOfMemory) => *control_flow = ControlFlow::Exit,
                //    // All other errors (Outdated, Timeout) should be resolved by the next frame
                //    Err(e) => eprintln!("{:?}", e),
                //}
            }
            Event::MainEventsCleared => {
                // RedrawRequested will only trigger once, unless we manually
                // request it.
                window.request_redraw();
            }
            _ => {}
        }
    });



    // Fake paint loop
    //for i in 0..10 {
    //};
    /*


    // Read out the noise texture, as a test
    let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
        label: None,
    });

    let output_buffer_size = (4 * texture_size * texture_size) as wgpu::BufferAddress;
    let output_buffer = device.create_buffer(&wgpu::BufferDescriptor {
        size: output_buffer_size,
        usage: wgpu::BufferUsage::COPY_DST
            | wgpu::BufferUsage::MAP_READ,
        label: None,
        mapped_at_creation: false,
    });

    let texture_extent = wgpu::Extent3d {
        width: texture_size as u32,
        height: texture_size as u32,
        depth: 1,
    };

    encoder.copy_texture_to_buffer(
        wgpu::TextureCopyView {
            texture: &texture.texture,
            mip_level: 0,
            origin: wgpu::Origin3d::ZERO,
        }, 
        wgpu::BufferCopyView {
            buffer: &output_buffer,
            layout: wgpu::TextureDataLayout {
                offset: 0,
                bytes_per_row: 4 * texture_size as u32,
                rows_per_image: texture_size as u32,
            },
        }, 
        texture_extent,
    );

    queue.submit(Some(encoder.finish()));

    // NOTE: We have to create the mapping THEN device.poll(). If we don't
    // the application will freeze.
    let buffer_slice = output_buffer.slice(..);
    let mapping = buffer_slice.map_async(wgpu::MapMode::Read);
    device.poll(wgpu::Maintain::Wait);

    block_on(mapping).unwrap();
    let data = buffer_slice.get_mapped_range();

    use image::{ImageBuffer, Rgba};
    let buffer = ImageBuffer::<Rgba<u8>, _>::from_raw(
        texture_size as u32,
        texture_size as u32,
        data,
    ).unwrap();

    buffer.save("image.png").unwrap();

    println!("Hello, world!");
    */
}
