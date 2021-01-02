use radiance;
use imgui::*;
use std::rc::Rc;
use radiance::imgui_wgpu;
use std::collections::{HashMap, HashSet};

#[derive(Debug)]
enum Node {
    EffectNode(radiance::EffectNode<radiance::DefaultContext, radiance::DefaultChain>),
}

#[derive(Debug, PartialEq, Eq, Hash)]
struct Edge {
    from: u32,
    to: u32,
    input: u32,
}

fn tile<UpdateContext: radiance::WorkerPool + radiance::FetchContent + radiance::Timebase, PaintContext: radiance::BlankTexture + radiance::NoiseTexture + radiance::Resolution>(ui: &imgui::Ui, renderer: &mut imgui_wgpu::Renderer, device: &wgpu::Device, id_str: &ImStr, effect_node: &mut radiance::EffectNode<UpdateContext, PaintContext>, tex: Rc<radiance::Texture>) {
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
            imgui::Image::new(renderer.texture_id(device, tex), [100.0, 100.0]).build(&ui);
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
    let preview_chain = Rc::new(ctx.new_chain(&ui.device, &ui.queue, (texture_size, texture_size)));

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

    let mut effect_node_droste = radiance::EffectNode::new();
    effect_node_droste.set_name(Some("droste.glsl"));
    nodes.insert(2, Node::EffectNode(effect_node_droste));

    edges.insert(Edge {
        from: 0,
        to: 1,
        input: 0,
    });

    edges.insert(Edge {
        from: 1,
        to: 2,
        input: 0,
    });

    let paint_contexts = vec![preview_chain.clone()];
    for node in nodes.values_mut() {
        match node {
            Node::EffectNode(n) => n.set_paint_contexts(&paint_contexts, &ui.device),
        }
    }

    ui.run_event_loop(event_loop, move |device, queue, imgui, mut renderer| {

        // Update context
        ctx.update();

        // Update nodes
        for node in nodes.values_mut() {
            match node {
                Node::EffectNode(n) => n.update(&ctx, device, queue),
            }
        }

        // TODO topo-sort
        let topo_order = [0, 1, 2];

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
                    let (node_cmds, output_tex) = n.paint(preview_chain.clone(), device, queue, input_textures.as_slice());
                    outputs.insert(node_id, output_tex);
                    node_cmds.into_iter()
                }
            }
        }).flatten().collect::<Vec<wgpu::CommandBuffer>>();
        // Not sure if this last collect() is strictly necessary,
        // but I want to avoid any potential for lazy evaluation.

        queue.submit(cmds);

        // Build the UI

        // Purge user textures so we don't leak them
        renderer.purge_user_textures();

        let ui = imgui.frame();
        let window = imgui::Window::new(im_str!("Hello Imgui from WGPU!"));
        window
            .size([600.0, 800.0], Condition::FirstUseEver)
            .build(&ui, || {
                ui.text(im_str!("Hello world!"));
                for node_id in &topo_order {
                    match nodes.get_mut(&node_id).unwrap() {
                        Node::EffectNode(effect_node) => {
                            tile(&ui, &mut renderer, &device, &im_str!("##tile{}", node_id), effect_node, outputs.get(&node_id).unwrap().clone());
                        }
                    };
                    ui.same_line(0.);
                }
            });
        ui
    });
}
