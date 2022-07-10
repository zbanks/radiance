use std::string::String;
use crate::context::{Context, ArcTextureViewSampler};
use crate::render_target::RenderTargetId;

pub struct EffectNodeProps {
    pub name: String,
    pub intensity: f32,
}

pub enum EffectNodeStatus {
}

pub enum EffectNodeState {
    Uninitialized,
    Ready(ReadyState),
    Error(String),
}

pub struct ReadyState {
    render_pipeline: wgpu::RenderPipeline,
    update_bind_group: wgpu::BindGroup,
    paint_bind_group_layout: wgpu::BindGroupLayout,
    update_uniform_buffer: wgpu::Buffer,
    paint_uniform_buffer: wgpu::Buffer,
    n_inputs: u32,
}

struct EffectNodePaintState {
//    width: u32,
//    height: u32,
//    texture: Arc<wgpu::Texture>,
}

impl EffectNodeState {
    pub fn new(ctx: &Context, props: &EffectNodeProps) -> Self {
        Self::Uninitialized
    }

    pub fn update(&mut self, ctx: &Context, props: &EffectNodeProps, time: f32) {
    }

    pub fn paint(&mut self, ctx: &Context, render_target_id: RenderTargetId) -> ArcTextureViewSampler {
        return ctx.render_target_state(render_target_id).unwrap().noise_texture().clone(); // XXX actually paint something
    }
}

//impl EffectNodePaintState {
//    pub fn new(width: u32, height: u32) -> EffectNodePaintState {
//        let texture_desc = wgpu::TextureDescriptor {
//            size: wgpu::Extent3d {
//                width,
//                height,
//                depth_or_array_layers: 1,
//            },
//            mip_level_count: 1,
//            sample_count: 1,
//            dimension: wgpu::TextureDimension::D2,
//            format: wgpu::TextureFormat::Rgba8UnormSrgb,
//            usage: wgpu::TextureUsages::COPY_SRC
//                | wgpu::TextureUsages::RENDER_ATTACHMENT
//                ,
//            label: None,
//        };
//        let texture = self.device.create_texture(&texture_desc);
//
//        EffectNodePaintState {
//            width,
//            height,
//            texture,
//        }
//    }
//
//}
