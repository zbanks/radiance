//use radiance::{DefaultContext, EffectNode, EffectNodeArguments};
use radiance;
use imgui::*;

fn main() {
    let (ui, event_loop) = radiance::ui::DefaultUI::setup();
    ui.run_event_loop(event_loop, move |device, queue, imgui| {
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
                //imgui::Image::new(purple_tex_id, [100.0, 100.0]).build(&ui);
            });
        ui
    });
}
