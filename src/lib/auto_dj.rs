use crate::effect_node::EffectNodeProps;
use crate::graph::{Edge, NodeId};
use crate::placeholder_node::PlaceholderNodeProps;
use crate::props::{NodeProps, Props};

use rand::{Rng, prelude::SliceRandom};

const STABLE_EFFECT_COUNT: usize = 6;

const STABLE_TIMER_MIN: usize = 100;
const STABLE_TIMER_MAX: usize = 300;
const CROSSFADE_TIMER: usize = 200;

#[derive(Debug)]
struct AutoDJEffectDescriptor {
    name: &'static str,
    category: AutoDJEffectCategory,
    intensity_min: f32,
    intensity_max: f32,
    random_frequency: bool,
}

#[derive(Debug, PartialEq)]
enum AutoDJEffectCategory {
    DontUse,
    Generative,
    Complecting,
    Simplifying,
}

const EFFECT_DESCRIPTOR_DEFAULT: AutoDJEffectDescriptor = AutoDJEffectDescriptor {
    name: "",
    category: AutoDJEffectCategory::DontUse,
    intensity_min: 0.2,
    intensity_max: 1.,
    random_frequency: true,
};

const EFFECTS: &[AutoDJEffectDescriptor] = &[
    AutoDJEffectDescriptor {name: "afix", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "afixhighlight", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "allwhite", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "attractor", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "bespeckle", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "bespecklep", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "blowout", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "brighten", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "broken", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "bstrobe", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "bumpsphere", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "bwave", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "cedge", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "cga1", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "chansep", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "circle", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "coin", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "color", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "composite", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "count", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "crack", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT}, // Doesn't blend well
    AutoDJEffectDescriptor {name: "crossfader", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "crt", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "csmooth", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT}, // Could use, maybe
    AutoDJEffectDescriptor {name: "cube", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "cyan", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "cycle2", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "cylinder", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "deblack", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "delace", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "delay", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "depolar", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "desat", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "diodefoh", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "diodelpf", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "distort", category: AutoDJEffectCategory::Complecting, random_frequency: false, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "droste", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "dunkirk", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "dwwave", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "edge", category: AutoDJEffectCategory::Simplifying, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "emboss", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "eye", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "filmstop", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "fire", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "fireball", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "flippy", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "flow", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "flower", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "flowing", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "fly", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "fractalzoom", category: AutoDJEffectCategory::Complecting, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "lpf", category: AutoDJEffectCategory::Simplifying, random_frequency: false, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "purple", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "test", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "uvmap", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "vu", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "wwave", category: AutoDJEffectCategory::Generative, ..EFFECT_DESCRIPTOR_DEFAULT},
    AutoDJEffectDescriptor {name: "zoomin", category: AutoDJEffectCategory::DontUse, ..EFFECT_DESCRIPTOR_DEFAULT}, // Could use, maybe
];

// Frequency options for randomly selected frequency
const RANDOM_FREQUENCY_OPTIONS: &[Option<f32>] = &[None, Some(0.25), Some(0.5), Some(1.)];

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
    timer: usize, // Countdown timer to crossfade
}

#[derive(Debug)]
struct AutoDJCrossfade {
    effects: [AutoDJEffect; STABLE_EFFECT_COUNT + 1],
    fade_in_ix: usize,
    fade_out_ix: usize,
    start_timer: usize, // What the timer started at
    timer: usize,       // Countdown timer to stable
}

impl AutoDJ {
    pub fn new() -> Self {
        AutoDJ {
            state: AutoDJState::Pending,
        }
    }

    fn pick_new_node(ix: usize) -> (NodeProps, AutoDJEffect) {
        let mut rng = rand::thread_rng();

        let descriptor_options = match ix {
            0 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Generative).collect::<Vec<_>>(),
            1 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Generative || e.category == AutoDJEffectCategory::Complecting).collect::<Vec<_>>(),
            2 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Complecting).collect::<Vec<_>>(),
            3 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Complecting || e.category == AutoDJEffectCategory::Simplifying).collect::<Vec<_>>(),
            4 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Complecting || e.category == AutoDJEffectCategory::Simplifying).collect::<Vec<_>>(),
            5 => EFFECTS.iter().filter(|e| e.category == AutoDJEffectCategory::Simplifying).collect::<Vec<_>>(),
            _ => panic!("Don't know how to handle AutoDJ slot {}", ix),
        };
        let descriptor = descriptor_options.choose(&mut rng).unwrap();

        let target_intensity = rng.gen_range(descriptor.intensity_min..descriptor.intensity_max);

        let frequency = if descriptor.random_frequency {
            RANDOM_FREQUENCY_OPTIONS.choose(&mut rng).unwrap().clone()
        } else {
            None
        };

        let id = NodeId::gen();
        let props = NodeProps::EffectNode(EffectNodeProps {
            name: descriptor.name.to_owned(),
            frequency,
            ..Default::default()
        });
        let effect = AutoDJEffect {
            id,
            target_intensity,
        };
        (props, effect)
    }

    fn stable_timer() -> usize {
        let mut rng = rand::thread_rng();
        rng.gen_range(STABLE_TIMER_MIN..STABLE_TIMER_MAX)
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
                        timer: Self::stable_timer(),
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
                            let mut rng = rand::thread_rng();

                            // Pick a node at random to fade out & replace
                            let fade_out_ix = rng.gen_range(0..STABLE_EFFECT_COUNT);
                            let (new_props, new_effect) = Self::pick_new_node(fade_out_ix);

                            // Randomly insert before or after the node we are replacing
                            let fade_in_ix = rng.gen_range(fade_out_ix..=fade_out_ix + 1);

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

                            let effects: [_; STABLE_EFFECT_COUNT + 1] = (0..STABLE_EFFECT_COUNT + 1)
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
                            let to = if fade_in_ix < STABLE_EFFECT_COUNT + 1 - 1 {
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
                                start_timer: CROSSFADE_TIMER,
                                timer: CROSSFADE_TIMER,
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
                            let to = if crossfade_state.fade_out_ix < STABLE_EFFECT_COUNT + 1 - 1 {
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
                                timer: Self::stable_timer(),
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
