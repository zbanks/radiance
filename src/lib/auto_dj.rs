use crate::effect_node::EffectNodeProps;
use crate::graph::{Edge, NodeId};
use crate::placeholder_node::PlaceholderNodeProps;
use crate::props::{NodeProps, Props};

const STABLE_EFFECT_COUNT: usize = 4;

#[derive(Debug)]
pub struct AutoDJ {
    state: AutoDJState,
}

#[derive(Debug)]
enum AutoDJState {
    Pending,
    Running(AutoDJRunning),
    Broken,
}

#[derive(Debug)]
struct AutoDJRunning {
    left_placeholder: NodeId,
    right_placeholder: NodeId,
    action: AutoDJAction,
}

#[derive(Debug)]
enum AutoDJAction {
    Stable(AutoDJStable),
    Crossfade(AutoDJCrossfade),
}

#[derive(Clone, Debug)]
struct AutoDJEffect {
    id: NodeId,
    target_intensity: f32,
}

#[derive(Debug)]
struct AutoDJStable {
    effects: [AutoDJEffect; STABLE_EFFECT_COUNT],
    timer: u32, // Countdown timer to crossfade
}

#[derive(Debug)]
struct AutoDJCrossfade {
    effects: [AutoDJEffect; STABLE_EFFECT_COUNT + 1],
    fade_in_ix: usize,
    fade_out_ix: usize,
    start_timer: u32, // What the timer started at
    timer: u32,       // Countdown timer to stable
}

impl AutoDJ {
    pub fn new() -> Self {
        AutoDJ {
            state: AutoDJState::Pending,
        }
    }

    fn pick_new_node(ix: usize) -> (NodeProps, AutoDJEffect) {
        let (name, target_intensity) = match ix {
            0 => ("wwave", 0.8),
            1 => ("chansep", 0.5),
            2 => ("bespeckle", 0.5),
            3 => ("cedge", 0.5),
            _ => panic!("Don't know how to pick a node for slot {}", ix),
        };
        let id = NodeId::gen();
        let props = NodeProps::EffectNode(EffectNodeProps {
            name: name.to_owned(),
            ..Default::default()
        });
        let effect = AutoDJEffect {
            id,
            target_intensity,
        };
        (props, effect)
    }

    pub fn update(&mut self, props: &mut Props) {
        match &mut self.state {
            AutoDJState::Pending => {
                let left_placeholder = NodeId::gen();
                let right_placeholder = NodeId::gen();
                props.node_props.insert(
                    left_placeholder,
                    NodeProps::PlaceholderNode(PlaceholderNodeProps {}),
                );
                props.graph.nodes.push(left_placeholder);
                let mut prev_id = left_placeholder;
                let effects: [_; STABLE_EFFECT_COUNT] = (0..STABLE_EFFECT_COUNT)
                    .map(|i| {
                        let (mut new_props, effect) = Self::pick_new_node(i);
                        if let NodeProps::EffectNode(e) = &mut new_props {
                            e.intensity = Some(effect.target_intensity); // Initialize nodes directly to their target intensity
                        }
                        props.node_props.insert(effect.id, new_props);
                        props.graph.nodes.push(effect.id);
                        props.graph.edges.push(Edge {
                            from: prev_id,
                            to: effect.id,
                            input: 0,
                        });
                        prev_id = effect.id;
                        effect
                    })
                    .collect::<Vec<_>>()
                    .try_into()
                    .unwrap();
                props.node_props.insert(
                    right_placeholder,
                    NodeProps::PlaceholderNode(PlaceholderNodeProps {}),
                );
                props.graph.nodes.push(right_placeholder);
                props.graph.edges.push(Edge {
                    from: prev_id,
                    to: right_placeholder,
                    input: 0,
                });
                self.state = AutoDJState::Running(AutoDJRunning {
                    left_placeholder,
                    right_placeholder,
                    action: AutoDJAction::Stable(AutoDJStable {
                        effects,
                        timer: 100,
                    }),
                });
            }
            AutoDJState::Running(running_state) => {
                match &mut running_state.action {
                    AutoDJAction::Stable(stable_state) => {
                        // Decrement the stable timer
                        // If we have hit 0, transition to crossfade
                        if stable_state.timer > 0 {
                            stable_state.timer -= 1;
                        } else {
                            let fade_out_ix = 1;
                            let (new_props, new_effect) = Self::pick_new_node(1);
                            let fade_in_ix = fade_out_ix; // Insert before; could be one more than fade_out_ix to insert after

                            // Remove edge from graph that inputs to fade_in_ix
                            let from = if fade_in_ix > 0 {
                                stable_state.effects[fade_in_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };

                            let to = if fade_in_ix < STABLE_EFFECT_COUNT {
                                stable_state.effects[fade_in_ix].id
                            } else {
                                running_state.right_placeholder
                            };

                            let delete_edge = Edge { from, to, input: 0 };
                            props.graph.edges.retain(|e| e != &delete_edge);

                            let effects: [_; 5] = (0..STABLE_EFFECT_COUNT + 1)
                                .map(|i| {
                                    if i < fade_in_ix {
                                        stable_state.effects[i].clone()
                                    } else if i == fade_in_ix {
                                        new_effect.clone()
                                    } else {
                                        stable_state.effects[i - 1].clone()
                                    }
                                })
                                .collect::<Vec<_>>()
                                .try_into()
                                .unwrap();
                            // Apply correction to fade_out_ix so it points to the right index in the new array
                            let fade_out_ix = if fade_in_ix <= fade_out_ix {
                                fade_out_ix + 1
                            } else {
                                fade_out_ix
                            };

                            // Add new node
                            props.node_props.insert(new_effect.id, new_props);
                            props.graph.nodes.push(new_effect.id);

                            // Add new left edge
                            let from = if fade_in_ix > 0 {
                                effects[fade_in_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };
                            props.graph.edges.push(Edge {
                                from,
                                to: new_effect.id,
                                input: 0,
                            });

                            // Add new right edge
                            let to = if fade_in_ix < STABLE_EFFECT_COUNT + 1 {
                                effects[fade_in_ix + 1].id
                            } else {
                                running_state.right_placeholder
                            };
                            props.graph.edges.push(Edge {
                                from: new_effect.id,
                                to,
                                input: 0,
                            });

                            // Transition state to crossfade
                            running_state.action = AutoDJAction::Crossfade(AutoDJCrossfade {
                                effects,
                                fade_in_ix,
                                fade_out_ix,
                                start_timer: 100,
                                timer: 100,
                            });
                        }
                    }
                    AutoDJAction::Crossfade(crossfade_state) => {
                        // Decrement the stable timer
                        // If we have hit 0, transition to stable
                        if crossfade_state.timer > 0 {
                            crossfade_state.timer -= 1;
                            let alpha =
                                crossfade_state.timer as f32 / crossfade_state.start_timer as f32;
                            let fade_in_effect =
                                &crossfade_state.effects[crossfade_state.fade_in_ix];
                            let mut fade_in_props =
                                props.node_props.get_mut(&fade_in_effect.id).unwrap(); // XXX don't unwrap; become broken if missing
                            if let NodeProps::EffectNode(e) = &mut fade_in_props {
                                e.intensity = Some(fade_in_effect.target_intensity * (1. - alpha));
                            }
                            let fade_out_effect =
                                &crossfade_state.effects[crossfade_state.fade_out_ix];
                            let mut fade_out_props =
                                props.node_props.get_mut(&fade_out_effect.id).unwrap(); // XXX don't unwrap; become broken if missing
                            if let NodeProps::EffectNode(e) = &mut fade_out_props {
                                e.intensity = Some(fade_out_effect.target_intensity * alpha);
                            }
                        } else {
                            let remove_id = crossfade_state.effects[crossfade_state.fade_out_ix].id;

                            // Remove node at fade_out_ix
                            props.node_props.remove(&remove_id);
                            props.graph.nodes.retain(|n| n != &remove_id);

                            // Remove both edges that connect to fade_out_ix
                            let from = if crossfade_state.fade_out_ix > 0 {
                                crossfade_state.effects[crossfade_state.fade_out_ix - 1].id
                            } else {
                                running_state.left_placeholder
                            };
                            let delete_left_edge = Edge {
                                from,
                                to: remove_id,
                                input: 0,
                            };
                            let to = if crossfade_state.fade_out_ix < STABLE_EFFECT_COUNT + 1 {
                                crossfade_state.effects[crossfade_state.fade_out_ix + 1].id
                            } else {
                                running_state.right_placeholder
                            };
                            let delete_right_edge = Edge {
                                from: remove_id,
                                to,
                                input: 0,
                            };
                            props
                                .graph
                                .edges
                                .retain(|e| e != &delete_left_edge && e != &delete_right_edge);

                            // Add bypass edge
                            props.graph.edges.push(Edge { from, to, input: 0 });

                            // Remove node from AutoDJ effects list
                            let effects = crossfade_state
                                .effects
                                .iter()
                                .enumerate()
                                .filter_map(|(i, e)| {
                                    if i == crossfade_state.fade_out_ix {
                                        None
                                    } else {
                                        Some(e.clone())
                                    }
                                })
                                .collect::<Vec<_>>()
                                .try_into()
                                .unwrap();

                            // Transition state to stable
                            running_state.action = AutoDJAction::Stable(AutoDJStable {
                                effects,
                                timer: 100,
                            });
                        }
                    }
                }
            }
            AutoDJState::Broken => {}
        }
    }

    pub fn is_broken(&self) -> bool {
        matches!(self.state, AutoDJState::Broken)
    }
}
