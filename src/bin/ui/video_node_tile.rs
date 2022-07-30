extern crate nalgebra as na;

use na::{Vector2, Matrix4};
use radiance::ArcTextureViewSampler;
use std::sync::Arc;

const MAX_N_INDICES: usize = 1024;
const MAX_N_VERTICES: usize = 1024;

// WTF nalgebra, where is your 2D cross product
fn cross(a: &Vector2<f32>, b: &Vector2<f32>) -> f32 {
    a.x * b.y - a.y * b.x
}

const BG_COLOR: [f32; 4] = [0., 0., 0., 1.];
const BORDER_COLOR: [f32; 4] = [0.3, 0.3, 0.3, 1.];
const BORDER_THICKNESS: f32 = 2.;
const CHEVRON_SIZE: f32 = 15.;

const EPSILON: f32 = 0.0001;

// The uniform buffer associated with the node tile
#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Uniforms {
    view: [[f32; 4]; 4],
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct Vertex {
    pos: [f32; 2],
    color: [f32; 4],
}

impl Vertex {
    fn desc<'a>() -> wgpu::VertexBufferLayout<'a> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[
                wgpu::VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: std::mem::size_of::<[f32; 2]>() as wgpu::BufferAddress,
                    shader_location: 1,
                    format: wgpu::VertexFormat::Float32x4,
                }
            ]
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

pub struct RenderPassResources {
    n_vertices: u32,
}

pub struct VideoNodeTileRenderer {
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    pipeline: wgpu::RenderPipeline,
    bind_group_layout: wgpu::BindGroupLayout,
    bind_group: wgpu::BindGroup,
    uniform_buffer: wgpu::Buffer,
    vertex_buffer: wgpu::Buffer,
    index_buffer: wgpu::Buffer,

    view: Matrix4<f32>,
    vertices: Vec<Vertex>,
    indices: Vec<u32>,
}

impl VideoNodeTileRenderer {
    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("video_node_tile.wgsl").into()),
        });

        let bind_group_layout = device.create_bind_group_layout(
            &wgpu::BindGroupLayoutDescriptor {
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 0,
                        visibility: wgpu::ShaderStages::VERTEX,
                        ty: wgpu::BindingType::Buffer {
                            ty: wgpu::BufferBindingType::Uniform,
                            has_dynamic_offset: false,
                            min_binding_size: None,
                        },
                        count: None,
                    },
                ],
                label: Some("screen bind group layout"),
            }
        );

        let pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                label: Some("Video Node Tile Pipeline Layout"),
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
            });

        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Video Node Tile Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: "vs_main",
                buffers: &[Vertex::desc()],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Bgra8UnormSrgb,
                    blend: Some(wgpu::BlendState::PREMULTIPLIED_ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: Some(wgpu::Face::Back),
                polygon_mode: wgpu::PolygonMode::Fill,
                unclipped_depth: false,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            multiview: None,
        });

        let uniform_buffer = device.create_buffer(
            &wgpu::BufferDescriptor {
                label: Some("video node tile uniform buffer"),
                size: std::mem::size_of::<Uniforms>() as u64,
                usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        let bind_group = device.create_bind_group(
            &wgpu::BindGroupDescriptor {
                layout: &bind_group_layout,
                entries: &[
                    wgpu::BindGroupEntry {
                        binding: 0,
                        resource: uniform_buffer.as_entire_binding(),
                    },
                ],
                label: Some("video node tile bind group"),
            }
        );

        let vertex_buffer = device.create_buffer(
            &wgpu::BufferDescriptor {
                label: Some("video node tile vertex buffer"),
                size: (std::mem::size_of::<Vertex>() * MAX_N_VERTICES) as u64,
                usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        let index_buffer = device.create_buffer(
            &wgpu::BufferDescriptor {
                label: Some("video node tile vertex buffer"),
                size: (std::mem::size_of::<Vertex>() * MAX_N_INDICES) as u64,
                usage: wgpu::BufferUsages::INDEX | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            }
        );

        let mut result = Self {
            device,
            queue,
            pipeline,
            bind_group_layout,
            uniform_buffer,
            bind_group,
            vertex_buffer,
            index_buffer,

            view: Matrix4::identity(),
            vertices: Default::default(),
            indices: Default::default(),
        };
        result.update_uniforms();
        result
    }

    pub fn set_view(&mut self, view: &Matrix4<f32>) {
        self.view = *view;
        self.update_uniforms();
    }

    fn update_uniforms(&mut self) {
        let uniforms = Uniforms {
            view: self.view.into(),
        };
        self.queue.write_buffer(&self.uniform_buffer, 0, bytemuck::cast_slice(&[uniforms]));
    }

    pub fn push_instance(&mut self, pos_min: &Vector2<f32>, pos_max: &Vector2<f32>, inputs: &[f32], outputs: &[f32]) {
        // Figure out how big each chevron can be without interfering with the
        // rectangle boundaries or other chevrons
        let calc_chevron_sizes = |locations: &[f32]| {
            (0..locations.len()).map(|i| {
                let pos = locations[i];
                let left_boundary = if i == 0 {
                    0.
                } else {
                    0.5 * (locations[i - 1] + locations[i])
                };
                let right_boundary = if i == locations.len() - 1 {
                    pos_max.y - pos_min.y
                } else {
                    0.5 * (locations[i] + locations[i + 1])
                };

                CHEVRON_SIZE.min(pos - left_boundary).min(right_boundary - pos).max(0.)
            }).collect::<Vec<f32>>()
        };

        let input_sizes = calc_chevron_sizes(inputs);
        let output_sizes = calc_chevron_sizes(outputs);

        // Construct the polygon in CCW order starting with the top left
        let mut vertices = Vec::<Vector2<f32>>::new();
        vertices.push(Vector2::<f32>::new(pos_min.x, pos_min.y));
        let mut last_loc = *inputs.first().unwrap_or(&f32::NAN);
        for (&loc, &size) in inputs.iter().zip(input_sizes.iter()) {
            assert!(loc >= last_loc, "Inputs are not sorted");
            if size > 0. {
                vertices.push(Vector2::<f32>::new(pos_min.x, pos_min.y + loc - size));
                vertices.push(Vector2::<f32>::new(pos_min.x + size, pos_min.y + loc));
                vertices.push(Vector2::<f32>::new(pos_min.x, pos_min.y + loc + size));
            }
            last_loc = loc;
        }
        vertices.push(Vector2::<f32>::new(pos_min.x, pos_max.y));
        vertices.push(Vector2::<f32>::new(pos_max.x, pos_max.y));
        let mut last_loc = *outputs.last().unwrap_or(&f32::NAN);
        for (&loc, &size) in outputs.iter().rev().zip(output_sizes.iter().rev()) {
            assert!(loc <= last_loc, "Outputs are not sorted");
            if size > 0. {
                vertices.push(Vector2::<f32>::new(pos_max.x, pos_min.y + loc + size));
                vertices.push(Vector2::<f32>::new(pos_max.x + size, pos_min.y + loc));
                vertices.push(Vector2::<f32>::new(pos_max.x, pos_min.y + loc - size));
            }
            last_loc = loc;
        }
        vertices.push(Vector2::<f32>::new(pos_max.x, pos_min.y));

        vertices.dedup_by(|a, b| (a.x - b.x).abs() < EPSILON && (a.y - b.y).abs() < EPSILON);

        // Push vertices of polygon
        let offset = self.vertices.len();
        for pos in vertices.iter() {
            self.vertices.push(Vertex {
                pos: [pos.x, pos.y],
                color: BG_COLOR,
            });
        }

        // Triangulate with ear clipping
        let mut indices: Vec<usize> = (0..vertices.len()).collect();
        while indices.len() > 2 {
            // Find an ear
            let mut found_interior = false;
            let mut found_exterior = false;
            for i in 0..indices.len() {
                let ii1 = (indices.len() + i - 1) % indices.len();
                let ii2 = i;
                let ii3 = (i + 1) % indices.len();
                let p1 = vertices[indices[ii1]];
                let p2 = vertices[indices[ii2]];
                let p3 = vertices[indices[ii3]];
                let c = cross(&(p2 - p1), &(p3 - p2));
                if c > EPSILON {
                    found_exterior = true;
                } else if c < -EPSILON {
                    // CCW polygon winding produces negative cross products on interior edges
                    found_interior = true;
                    self.indices.push((offset + indices[ii1]) as u32);
                    self.indices.push((offset + indices[ii2]) as u32);
                    self.indices.push((offset + indices[ii3]) as u32);
                    indices.remove(i); // Clip
                    break;
                }
            }
            if !found_interior && !found_exterior {
                // We have clipped the polygon to the point that it is degenerate
                // (has no area.)
                // I'd call that a success.
                break;
            }
            assert!(found_interior || !found_exterior, "Triangulation failed (malformed polygon?)");
        }

        // Now work on the border
        let offset = self.vertices.len();

        for i in 0..vertices.len() {
            let prev = vertices[(i + vertices.len() - 1) % vertices.len()];
            let cur = vertices[i];
            let next = vertices[(i + 1) % vertices.len()];

            // Polygon offset algorithm from https://stackoverflow.com/a/54042831
            let a = cur - prev;
            let b = next - cur;
            let na = Vector2::new(-a.y, a.x).normalize();
            let nb = Vector2::new(-b.y, b.x).normalize();
            let bis = (na + nb).normalize();
            // I think the SO length math was wrong
            let length = 0.5 * BORDER_THICKNESS / bis.dot(&na);
            let inset_vertex = cur - bis * length;
            let outset_vertex = cur + bis * length;

            // Push vertices
            self.vertices.push(Vertex {
                pos: [inset_vertex.x, inset_vertex.y],
                color: BORDER_COLOR,
            });
            self.vertices.push(Vertex {
                pos: [outset_vertex.x, outset_vertex.y],
                color: BORDER_COLOR,
            });

            // Push indices to draw a quad
            let cur_inset = (offset + i * 2) as u32;
            let cur_outset = cur_inset + 1;
            let next_inset = (offset + ((i + 1) % vertices.len()) * 2) as u32;
            let next_outset = next_inset + 1;
            self.indices.push(cur_inset);
            self.indices.push(cur_outset);
            self.indices.push(next_inset);
            self.indices.push(cur_outset);
            self.indices.push(next_outset);
            self.indices.push(next_inset);
        }
    }

    // Modeled off of https://github.com/gfx-rs/wgpu-rs/wiki/Encapsulating-Graphics-Work

    pub fn prepare(&mut self) -> RenderPassResources {
        // TODO recreate buffers if too big or too small

        self.queue.write_buffer(&self.vertex_buffer, 0, bytemuck::cast_slice(&self.vertices[..]));
        self.queue.write_buffer(&self.index_buffer, 0, bytemuck::cast_slice(&self.indices[..]));

        let result = RenderPassResources {
            n_vertices: self.indices.len() as u32,
        };

        // Clear the list for next time
        self.vertices.clear();
        self.indices.clear();

        result
    }

    pub fn paint<'rpass>(&'rpass self, render_pass: &mut wgpu::RenderPass<'rpass>, render_pass_resources: &'rpass RenderPassResources) {
        render_pass.set_pipeline(&self.pipeline);
        render_pass.set_bind_group(0, &self.bind_group, &[]);
        render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
        render_pass.set_index_buffer(self.index_buffer.slice(..), wgpu::IndexFormat::Uint32);
        render_pass.draw_indexed(0..render_pass_resources.n_vertices, 0, 0..1);
    }
}
