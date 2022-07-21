@group(0) @binding(0)
var t_preview: texture_2d<f32>;

@group(0) @binding(1)
var s_preview: sampler;

struct InstanceInput {
    @location(0) pos_min: vec2<f32>,
    @location(1) pos_max: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(
    @builtin(vertex_index) vertex_index: u32,
    instance: InstanceInput,
) -> VertexOutput {

    var out: VertexOutput;

    var vertex_positions = array<vec2<f32>, 4>(
      vec2<f32>(0., 0.),
      vec2<f32>(1., 0.),
      vec2<f32>(0., 1.),
      vec2<f32>(1., 1.)
    );

    var vertex_uvs = array<vec2<f32>, 4>(
      vec2<f32>(0., 1.),
      vec2<f32>(1., 1.),
      vec2<f32>(0., 0.),
      vec2<f32>(1., 0.)
    );

    let position = vertex_positions[vertex_index];
    let uv = vertex_uvs[vertex_index];
    let position = instance.pos_min + position * (instance.pos_max - instance.pos_min);
    out.position = vec4<f32>(position.x, position.y, 0., 1.);
    out.uv = uv;
    return out;
}

// Fragment shader

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(t_preview, s_preview, vertex.uv);
}
