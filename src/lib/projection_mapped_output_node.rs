use crate::context::{ArcTextureViewSampler, Context};
use crate::render_target::RenderTargetId;
use crate::screen_output_node::AvailableOutputScreen;
use crate::CommonNodeProps;
use nalgebra::{Matrix3, Vector2};
use serde::{Deserialize, Serialize};

/// A single projector output
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct ProjectionMappedScreen {
    /// Name of the display output
    pub name: String,

    /// Resolution of this screen
    pub resolution: [u32; 2],

    /// A polygon in screen space defining the region to display
    pub crop: Vec<Vector2<f32>>,

    /// A 2D projection matrix defining the mapping
    /// from global coordinates to this screen's local coordinates
    pub map: Matrix3<f32>,
}

/// Properties of a ProjectionMappedOutputNode.
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ProjectionMappedOutputNodeProps {
    #[serde(default)]
    pub visible: bool,
    #[serde(default)]
    pub available_screens: Vec<AvailableOutputScreen>,
    #[serde(default)]
    pub screens: Vec<ProjectionMappedScreen>,
    pub resolution: [u32; 2],
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
