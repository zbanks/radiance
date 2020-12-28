use radiance;
use imgui::*;
use std::rc::Rc;
use radiance::imgui_wgpu;

fn tile<T: radiance::WorkerPool + radiance::FetchContent + radiance::Timebase>(ui: &imgui::Ui, id_str: &ImStr, effect_node: &mut radiance::EffectNode<T>, tex_id: imgui::TextureId) {
    const GRAY_BG: [f32; 4] = [0.1, 0.1, 0.1, 1.0];
    let color = ui.push_style_color(StyleColor::ChildBg, GRAY_BG);
    let mut intensity: f32 = effect_node.intensity();
    imgui::ChildWindow::new(id_str)
        .size([130., 200.])
        .border(true)
        .build(&ui, || {
            let im_name_str = im_str!("{}", effect_node.name().unwrap_or("(none)"));
            let text_width = ui.calc_text_size(&im_name_str, false, 150.)[0];
            ui.set_cursor_pos([0.5 * (ui.window_size()[0] - text_width), ui.cursor_pos()[1]]);
            ui.text(im_name_str);
            ui.separator();
            ui.set_cursor_pos([0.5 * (ui.window_size()[0] - 100.0), ui.cursor_pos()[1]]);
            imgui::Image::new(tex_id, [100.0, 100.0]).build(&ui);
            ui.set_next_item_width(ui.content_region_avail()[0]);
            imgui::Slider::new(im_str!("##slider"))
                .range(0. ..= 1.)
                .display_format(im_str!("%0.2f"))
                .build(ui, &mut intensity);
        });
    color.pop(&ui);
    effect_node.set_intensity(intensity);
}

fn main() {
    let (ui, event_loop) = radiance::ui::DefaultUI::setup();

    // Create a radiance Context
    let mut ctx = radiance::DefaultContext::new(&ui.device, &ui.queue);

    let texture_size = 256;
    let test_chain_id = ctx.add_chain(&ui.device, &ui.queue, (texture_size, texture_size));
    let mut effect_node_purple = radiance::EffectNode::new();
    effect_node_purple.set_name(Some("purple.glsl"));
    effect_node_purple.set_intensity(1.);

    let mut effect_node_droste = radiance::EffectNode::new();
    effect_node_droste.set_name(Some("droste.glsl"));

    let chain = ctx.chain(test_chain_id).unwrap();
    let mut paint_state_purple = effect_node_purple.new_paint_state(chain, &ui.device);
    let mut paint_state_droste = effect_node_droste.new_paint_state(chain, &ui.device);

    let mut purple_tex_id = None;
    let mut droste_tex_id = None;

    ui.run_event_loop(event_loop, move |device, queue, imgui, renderer| {
        // Update context
        ctx.update();
        let chain = ctx.chain(test_chain_id).unwrap();

        // Update and render effect node
        effect_node_purple.update(&ctx, device, queue);
        effect_node_droste.update(&ctx, device, queue);
        let (cmds_purple, tex_purple) = effect_node_purple.paint(chain, device, queue, &mut paint_state_purple, &[]);
        let (cmds_droste, tex_droste) = effect_node_droste.paint(chain, device, queue, &mut paint_state_droste, &[Some(tex_purple.clone())]);
        queue.submit(cmds_purple.into_iter().chain(cmds_droste.into_iter()));

        if let Some(id) = purple_tex_id {
            let existing_texture = &renderer.textures.get(id).unwrap().texture;
            if !Rc::ptr_eq(&tex_purple, existing_texture) {
                renderer.textures.replace(id, imgui_wgpu::Texture::from_radiance(tex_purple.clone(), device, renderer));
            }
        } else {
            purple_tex_id = Some(renderer.textures.insert(imgui_wgpu::Texture::from_radiance(tex_purple.clone(), device, renderer)));
        }
        if let Some(id) = droste_tex_id {
            let existing_texture = &renderer.textures.get(id).unwrap().texture;
            if !Rc::ptr_eq(&tex_droste, existing_texture) {
                renderer.textures.replace(id, imgui_wgpu::Texture::from_radiance(tex_droste.clone(), device, renderer));
            }
        } else {
            droste_tex_id = Some(renderer.textures.insert(imgui_wgpu::Texture::from_radiance(tex_droste.clone(), device, renderer)));
        }

        // Build the UI
        let ui = imgui.frame();
        let window = imgui::Window::new(im_str!("Hello Imgui from WGPU!"));
        window
            .size([600.0, 800.0], Condition::FirstUseEver)
            .build(&ui, || {
                ui.text(im_str!("Hello world!"));
                tile(&ui, im_str!("##tile1"), &mut effect_node_purple, purple_tex_id.unwrap());
                ui.same_line(0.);
                tile(&ui, im_str!("##tile2"), &mut effect_node_droste, droste_tex_id.unwrap());
            });
        ui
    });
}
