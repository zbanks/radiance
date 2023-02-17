use crate::context::{Context, ArcTextureViewSampler};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use serde::{Serialize, Deserialize};

/// Properties of a ScreenOutputNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ScreenOutputNodeProps {
    #[serde(default)]
    pub visible: bool,
    #[serde(default)]
    pub screen: String,
    #[serde(default)]
    pub available_screens: Vec<String>,
}

impl From<&ScreenOutputNodeProps> for CommonNodeProps {
    fn from(props: &ScreenOutputNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

pub struct ScreenOutputNodeState {
}

impl ScreenOutputNodeState {
    pub fn new(ctx: &Context, props: &ScreenOutputNodeProps) -> Self {
        Self {}
    }

    pub fn update(&mut self, ctx: &Context, props: &mut ScreenOutputNodeProps) {
    }

    pub fn paint(&mut self, ctx: &Context, encoder: &mut wgpu::CommandEncoder, render_target_id: RenderTargetId, inputs: &[Option<ArcTextureViewSampler>]) -> ArcTextureViewSampler {
        inputs.first().cloned().flatten().unwrap_or_else(|| ctx.blank_texture().clone())
    }
}
