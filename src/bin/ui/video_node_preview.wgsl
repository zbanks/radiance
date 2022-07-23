// A shader for drawing the video node preview
// (typically rendering a texture over top of a checkerboard to indicate transparency)

struct Uniforms {
    view: mat4x4<f32>,
    pos_min: vec2<f32>,
    pos_max: vec2<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(0) @binding(1)
var t_preview: texture_2d<f32>;

@group(0) @binding(2)
var s_preview: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(
    @builtin(vertex_index) vertex_index: u32,
) -> VertexOutput {

    var out: VertexOutput;

    // This shader generates its own vertex coordinates.
    // Just draw 4 vertices in a triangle strip.

    var vertex_uvs = array<vec2<f32>, 4>(
      vec2<f32>(0., 1.),
      vec2<f32>(1., 1.),
      vec2<f32>(0., 0.),
      vec2<f32>(1., 0.)
    );

    out.uv = vertex_uvs[vertex_index];
    let position = uniforms.pos_min + out.uv * (uniforms.pos_max - uniforms.pos_min);
    out.position = uniforms.view * vec4<f32>(position.x, position.y, 0., 1.);
    return out;
}

// Fragment shader

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    // TODO render checkerboard under texture
    return textureSample(t_preview, s_preview, vertex.uv);
}
