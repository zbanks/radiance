extern crate nalgebra as na;
use std::sync::Arc;
use na::{Matrix4, Vector2};
use radiance::ArcTextureViewSampler;

use crate::ui::video_node_preview::VideoNodePreviewRenderer;
use crate::ui::video_node_tile::VideoNodeTileRenderer;

/// A container holding all of the special-purpose renderers that go into drawing the UI
pub struct Renderer {
    _device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,

    video_node_preview_renderer: VideoNodePreviewRenderer,
    video_node_tile_renderer: VideoNodeTileRenderer,
}

impl Renderer {
    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        Self {
            _device: device.clone(),
            queue: queue.clone(),
            video_node_preview_renderer: VideoNodePreviewRenderer::new(device.clone(), queue.clone()),
            video_node_tile_renderer: VideoNodeTileRenderer::new(device, queue),
        }
    }

    fn view_matrix(width: u32, height: u32) -> Matrix4<f32> {
        const PIXEL_SCALING: f32 = 2.;
        let m = Matrix4::<f32>::new(
            2.0 / width as f32, 0., 0., -1.,
            0., -2.0 / height as f32, 0., 1.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
        );

        let m = m * Matrix4::<f32>::new(
            PIXEL_SCALING, 0., 0., 0.,
            0., PIXEL_SCALING, 0., 0.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
        );
        m
    }

    pub fn resize(&mut self, width: u32, height: u32) {
        let view = Self::view_matrix(width, height);
        self.video_node_preview_renderer.set_view(&view);
        self.video_node_tile_renderer.set_view(&view);
    }

    pub fn render(&mut self, surface: &wgpu::Surface, mut encoder: wgpu::CommandEncoder) -> Result<(), wgpu::SurfaceError> {
        let output = surface.get_current_texture()?;
        let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

        let video_node_tile_resources = self.video_node_tile_renderer.prepare();
        let video_node_preview_resources = self.video_node_preview_renderer.prepare();

        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Render Pass"),
                color_attachments: &[
                    Some(wgpu::RenderPassColorAttachment {
                        view: &view,
                        resolve_target: None,
                        ops: wgpu::Operations {
                            load: wgpu::LoadOp::Clear(
                                wgpu::Color {
                                    r: 0.1,
                                    g: 0.2,
                                    b: 0.3,
                                    a: 1.0,
                                }
                            ),
                            store: true,
                        }
                    })
                ],
                depth_stencil_attachment: None,
            });

            self.video_node_tile_renderer.paint(&mut render_pass, &video_node_tile_resources);
            self.video_node_preview_renderer.paint(&mut render_pass, &video_node_preview_resources);
        }

        self.queue.submit(std::iter::once(encoder.finish()));
        output.present();
        Ok(())
    }

    pub fn effect_node(&mut self, _state: &radiance::EffectNodeState, texture: &ArcTextureViewSampler, pos_min: &Vector2<f32>, pos_max: &Vector2<f32>) {
        let width = pos_max.x - pos_min.x;
        let height = pos_max.y - pos_min.y;
        self.video_node_tile_renderer.push_instance(pos_min, pos_max, &[0.5 * height], &[0.5 * height]);

        let preview_center = 0.5 * (pos_min + pos_max);
        let preview_size = (0.5 * width - 20.).min(0.5 * height - 30.);
        let preview_size = Vector2::<f32>::new(preview_size, preview_size);
        self.video_node_preview_renderer.push_instance(texture, &(preview_center - preview_size), &(preview_center + preview_size));
    }
}
