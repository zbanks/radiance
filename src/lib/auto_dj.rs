use crate::graph::{Edge, NodeId};
use crate::placeholder_node::PlaceholderNodeProps;
use crate::props::{NodeProps, Props};

pub struct AutoDJ {
    state: AutoDJState,
}

enum AutoDJState {
    Pending,
    Running(RunningAutoDJ),
    Broken,
}

pub struct RunningAutoDJ {
    left_placeholder: NodeId,
    right_placeholder: NodeId,
}

impl AutoDJ {
    pub fn new() -> Self {
        AutoDJ {
            state: AutoDJState::Pending,
        }
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
                props.node_props.insert(
                    right_placeholder,
                    NodeProps::PlaceholderNode(PlaceholderNodeProps {}),
                );
                props.graph.nodes.push(left_placeholder);
                props.graph.nodes.push(right_placeholder);
                props.graph.edges.push(Edge {
                    from: left_placeholder,
                    to: right_placeholder,
                    input: 0,
                });
                self.state = AutoDJState::Running(RunningAutoDJ {
                    left_placeholder,
                    right_placeholder,
                });
            }
            AutoDJState::Running(_) => {}
            AutoDJState::Broken => {}
        }
    }

    pub fn is_broken(&self) -> bool {
        matches!(self.state, AutoDJState::Broken)
    }
}
