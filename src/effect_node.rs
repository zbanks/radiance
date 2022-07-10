use std::string::String;
use crate::context::{Context, ArcTextureViewSampler};
use crate::render_target::RenderTargetId;

pub struct EffectNodeProps {
    pub name: String,
    pub intensity: f32,
}

pub struct EffectNodeState {
}

struct EffectNodePaintState {
//    width: u32,
//    height: u32,
//    texture: Arc<wgpu::Texture>,
}

impl EffectNodeState {
    pub fn new(ctx: &Context, props: &EffectNodeProps) -> Self {
        Self {}
    }

    pub fn paint(&mut self, ctx: &Context, render_target_id: RenderTargetId, props: &EffectNodeProps, time: f32) -> ArcTextureViewSampler {
        return ctx.blank_texture().clone(); // XXX actually paint something
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
