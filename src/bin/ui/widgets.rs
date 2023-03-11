use egui::{TextureId, Vec2};
use egui_wgpu::renderer::Renderer;
use radiance::{ArcTextureViewSampler, AudioLevels};
use rand::Rng;
use std::sync::Arc;

pub struct Widgets {
    // Constructor arguments:
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    pixels_per_point: f32,

    // Internal state:
    waveform_width: u32,
    waveform_height: u32,
    waveform_texture: ArcTextureViewSampler,
}

impl Widgets {
    fn make_texture(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        width: u32,
        height: u32,
    ) -> ArcTextureViewSampler {
        let texture_size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };

        let texture_desc = wgpu::TextureDescriptor {
            size: texture_size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::COPY_DST // XXX
                | wgpu::TextureUsages::RENDER_ATTACHMENT
                | wgpu::TextureUsages::TEXTURE_BINDING,
            label: None,
        };

        let texture = device.create_texture(&texture_desc);

        // Begin XXX
        let random_bytes: Vec<u8> = (0..width * height * 4)
            .map(|_| rand::thread_rng().gen::<u8>())
            .collect();

        queue.write_texture(
            wgpu::ImageCopyTexture {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &random_bytes,
            wgpu::ImageDataLayout {
                offset: 0,
                bytes_per_row: std::num::NonZeroU32::new(4 * width),
                rows_per_image: std::num::NonZeroU32::new(height),
            },
            texture_size,
        );
        // End XXX

        let view = texture.create_view(&Default::default());
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });
        ArcTextureViewSampler::new(texture, view, sampler)
    }

    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>, pixels_per_point: f32) -> Self {
        let waveform_width = 1;
        let waveform_height = 1;
        let waveform_texture = Self::make_texture(&device, &queue, waveform_width, waveform_height);
        Self {
            device,
            queue,
            pixels_per_point,

            waveform_width,
            waveform_height,
            waveform_texture,
        }
    }

    pub fn waveform(
        &mut self,
        renderer: &mut Renderer,
        size: Vec2,
        audio: AudioLevels,
    ) -> TextureId {
        // See if we need to remake the texture
        let width = (size.x * self.pixels_per_point) as u32;
        let height = (size.y * self.pixels_per_point) as u32;
        if width != self.waveform_width || height != self.waveform_height {
            self.waveform_width = width;
            self.waveform_height = height;
            self.waveform_texture = Self::make_texture(&self.device, &self.queue, width, height);
        }

        renderer.register_native_texture(
            &self.device,
            &self.waveform_texture.view,
            wgpu::FilterMode::Linear,
        )
    }
}
