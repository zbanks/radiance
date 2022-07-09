use crate::{EffectNodeArgs, EffectNodeProps, EffectNodeState, EffectNodePaintState};

pub type NodeId = u128;

pub enum NodeArgs {
    /// Node args are construction arguments passed to a node upon its creation
    EffectNode(EffectNodeArgs),
}

pub enum NodeProps {
    /// Node props are per-node rendering inputs
    /// that are passed in every frame
    /// in a rest-ful style.

    EffectNode(EffectNodeProps),
}

pub enum NodeState {
    /// Node state is per-node state
    /// that is independent of render chain (resolution)

    EffectNode(EffectNodeState),
}

pub enum NodePaintState {
    /// Node paint state is state dependent on both
    /// the node and its render chain (resolution)

    EffectNode(EffectNodePaintState),
}
