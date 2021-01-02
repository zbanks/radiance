use radiance;
use imgui::*;
use std::rc::Rc;
use radiance::imgui_wgpu;
use std::collections::{HashMap, HashSet};

#[derive(Debug)]
enum Node {
    EffectNode(radiance::EffectNode<radiance::DefaultContext>),
}

#[derive(Debug, PartialEq, Eq, Hash)]
struct Edge {
    from: u32,
    to: u32,
    input: u32,
}

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

    // Create the preview chain
    let texture_size = 256;
    let preview_chain = ctx.new_chain(&ui.device, &ui.queue, (texture_size, texture_size));

    // Create the map of nodes
    let mut nodes = HashMap::<u32, Node>::new();
    let mut edges = HashSet::<Edge>::new();

    let mut effect_node_purple = radiance::EffectNode::new();
    effect_node_purple.set_name(Some("purple.glsl"));
    effect_node_purple.set_intensity(1.);
    nodes.insert(0, Node::EffectNode(effect_node_purple));

    let mut effect_node_droste = radiance::EffectNode::new();
    effect_node_droste.set_name(Some("droste.glsl"));
    nodes.insert(1, Node::EffectNode(effect_node_droste));

    edges.insert(Edge {
        from: 0,
        to: 1,
        input: 0,
    });

    let paint_contexts = vec![&preview_chain];
    for node in nodes.values_mut() {
        match node {
            Node::EffectNode(n) => n.set_paint_contexts(&paint_contexts, &ui.device),
        }
    }

    let mut purple_tex_id = None;
    let mut droste_tex_id = None;

    ui.run_event_loop(event_loop, move |device, queue, imgui, renderer| {

        // Update context
        ctx.update();

        // Update nodes
        for node in nodes.values_mut() {
            match node {
                Node::EffectNode(n) => n.update(&ctx, device, queue),
            }
        }

        // TODO topo-sort
        let topo_order = [0, 1];

        fn node_inputs(edges: &HashSet<Edge>) -> HashMap<u32, Vec<Option<u32>>> {
            let mut inputs = HashMap::new();
            for edge in edges {
                let input_vec = inputs.entry(edge.to).or_insert(Vec::new());
                // Grow the vector if necessary
                for _ in input_vec.len()..=(edge.input as usize) {
                    input_vec.push(None);
                }
                input_vec[edge.input as usize] = Some(edge.from);
            }
            inputs
        }

        let inputs = node_inputs(&edges);

        let mut outputs = HashMap::new();
        let cmds = topo_order.iter().map(|&node_id| {
            let node = nodes.get_mut(&node_id).unwrap();
            let input_textures = inputs.get(&node_id).unwrap_or(&vec![]).into_iter().map(|input_node_id| {
                input_node_id
                    .and_then(|input_node_id| outputs.get(&input_node_id)
                    .and_then(|input_node_ref: &Rc<radiance::Texture>| Some(input_node_ref.clone())))
            }).collect::<Vec<Option<Rc<radiance::Texture>>>>();
            match node {
                Node::EffectNode(n) => {
                    let (node_cmds, output_tex) = n.paint(&preview_chain, device, queue, input_textures.as_slice());
                    outputs.insert(node_id, output_tex);
                    node_cmds.into_iter()
                }
            }
        }).flatten().collect::<Vec<wgpu::CommandBuffer>>();
        // Not sure if this last collect() is strictly necessary...

        queue.submit(cmds);

        let tex_purple = outputs.get(&0).unwrap();
        let tex_droste = outputs.get(&1).unwrap();

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
                {
                    let effect_node_purple = match nodes.get_mut(&0).unwrap() {
                        Node::EffectNode(n) => n
                    };
                    tile(&ui, im_str!("##tile1"), effect_node_purple, purple_tex_id.unwrap());
                }
                ui.same_line(0.);
                {
                    let effect_node_droste = match nodes.get_mut(&1).unwrap() {
                        Node::EffectNode(n) => n
                    };
                    tile(&ui, im_str!("##tile2"), effect_node_droste, droste_tex_id.unwrap());
                }
            });
        ui
    });
}
