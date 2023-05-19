use crate::context::{ArcTextureViewSampler, Context};
use crate::render_target::RenderTargetId;
use crate::screen_output_node::AvailableOutputScreen;
use crate::CommonNodeProps;
use serde::{Deserialize, Serialize};

/// Properties of a ProjectionMappedOutputNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ProjectionMappedOutputNodeProps {
    #[serde(default)]
    pub visible: bool,
    #[serde(default)]
    pub available_screens: Vec<AvailableOutputScreen>,
}

impl From<&ProjectionMappedOutputNodeProps> for CommonNodeProps {
    fn from(_props: &ProjectionMappedOutputNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

pub struct ProjectionMappedOutputNodeState {}

impl ProjectionMappedOutputNodeState {
    pub fn new(_ctx: &Context, _props: &ProjectionMappedOutputNodeProps) -> Self {
        Self {}
    }

    pub fn update(&mut self, _ctx: &Context, _props: &mut ProjectionMappedOutputNodeProps) {}

    pub fn paint(
        &mut self,
        ctx: &Context,
        _encoder: &mut wgpu::CommandEncoder,
        _render_target_id: RenderTargetId,
        inputs: &[Option<ArcTextureViewSampler>],
    ) -> ArcTextureViewSampler {
        inputs
            .first()
            .cloned()
            .flatten()
            .unwrap_or_else(|| ctx.blank_texture().clone())
    }
}
