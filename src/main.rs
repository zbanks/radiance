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

    let mut args = radiance::EffectNodeArguments {
        name: Some("purple.glsl"),
        intensity: 1.,
    };

    ui.run_event_loop(event_loop, move |device, queue, imgui, renderer| {
        // Update context
        ctx.update();
        let chain = ctx.chain(test_chain_id).unwrap();

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
            .size([600.0, 800.0], Condition::FirstUseEver)
            .build(&ui, || {
                ui.text(im_str!("Hello world!"));
                const GRAY_BG: [f32; 4] = [0.1, 0.1, 0.1, 1.0];
                let color = ui.push_style_color(StyleColor::ChildBg, GRAY_BG);
                imgui::ChildWindow::new(im_str!("##test_tile"))
                    .size([130., 200.])
                    .border(true)
                    .build(&ui, || {
                        let text_width = ui.calc_text_size(im_str!("Purple"), false, 150.)[0];
                        ui.set_cursor_pos([0.5 * (ui.window_size()[0] - text_width), ui.cursor_pos()[1]]);
                        ui.text(im_str!("Purple"));
                        ui.separator();
                        ui.set_cursor_pos([0.5 * (ui.window_size()[0] - 100.0), ui.cursor_pos()[1]]);
                        imgui::Image::new(purple_tex_id.unwrap(), [100.0, 100.0]).build(&ui);
                        ui.set_next_item_width(ui.content_region_avail()[0]);
                        imgui::Slider::new(im_str!("##test_slider"))
                            .range(0. ..= 1.)
                            .display_format(im_str!("%0.2f"))
                            .build(&ui, &mut args.intensity);
                    });
                color.pop(&ui);
            });
        ui
    });
}
