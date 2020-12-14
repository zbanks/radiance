use radiance;
use imgui::*;
use std::rc::Rc;
use radiance::imgui_wgpu;

fn main() {
    let (ui, event_loop) = radiance::ui::DefaultUI::setup();

    // Create a radiance Context
    let mut ctx = radiance::DefaultContext::new(&ui.device, &ui.queue);

    let texture_size = 256;
    let test_chain_id = ctx.add_chain(&ui.device, &ui.queue, (texture_size, texture_size));
    let mut effect_node = radiance::EffectNode::new();
    let chain = ctx.chain(test_chain_id).unwrap();
    let mut paint_state = effect_node.new_paint_state(chain, &ui.device);

    let mut purple_tex_id = None;

    ui.run_event_loop(event_loop, move |device, queue, imgui, renderer| {
        // Update context
        ctx.update();
        let chain = ctx.chain(test_chain_id).unwrap();

        let args = radiance::EffectNodeArguments {
            name: Some("purple.glsl"),
        };

        // Update and render effect node
        effect_node.update(&ctx, device, queue, &args);
        let (cmds, tex) = effect_node.paint(chain, device, &mut paint_state);
        queue.submit(cmds);

        if let Some(id) = purple_tex_id {
            let existing_texture = &renderer.textures.get(id).unwrap().texture;
            if !Rc::ptr_eq(&tex, existing_texture) {
                renderer.textures.replace(id, imgui_wgpu::Texture::from_radiance(tex.clone(), device, renderer));
            }
        } else {
            purple_tex_id = Some(renderer.textures.insert(imgui_wgpu::Texture::from_radiance(tex.clone(), device, renderer)));
        }

        // Build the UI
        let ui = imgui.frame();
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
                imgui::Image::new(purple_tex_id.unwrap(), [100.0, 100.0]).build(&ui);
            });
        ui
    });
}
