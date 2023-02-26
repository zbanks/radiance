use crate::context::{ArcTextureViewSampler, Context};
use crate::render_target::RenderTargetId;
use crate::CommonNodeProps;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct AvailableOutputScreen {
    pub name: String,
    pub suggested_resolutions: Vec<[u32; 2]>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct SelectedOutputScreen {
    pub name: String,
    pub resolution: [u32; 2],
}

/// Properties of a ScreenOutputNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ScreenOutputNodeProps {
    #[serde(default)]
    pub visible: bool,
    #[serde(default)]
    pub screen: Option<SelectedOutputScreen>,
    #[serde(default)]
    pub available_screens: Vec<AvailableOutputScreen>,
}

impl From<&ScreenOutputNodeProps> for CommonNodeProps {
    fn from(_props: &ScreenOutputNodeProps) -> Self {
        CommonNodeProps {
            input_count: Some(1),
        }
    }
}

pub struct ScreenOutputNodeState {}

impl ScreenOutputNodeState {
    pub fn new(_ctx: &Context, _props: &ScreenOutputNodeProps) -> Self {
        Self {}
    }

    pub fn update(&mut self, _ctx: &Context, _props: &mut ScreenOutputNodeProps) {}

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
