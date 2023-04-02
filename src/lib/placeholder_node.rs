use crate::context::{ArcTextureViewSampler, Context};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use serde::{Deserialize, Serialize};

/// Properties of a PlaceholderNode
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct PlaceholderNodeProps {}

impl From<&PlaceholderNodeProps> for CommonNodeProps {
    fn from(_props: &PlaceholderNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

pub struct PlaceholderNodeState {}

impl PlaceholderNodeState {
    pub fn new(_ctx: &Context, _props: &PlaceholderNodeProps) -> Self {
        Self {}
    }

    pub fn update(&mut self, _ctx: &Context, _props: &mut PlaceholderNodeProps) {}

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
