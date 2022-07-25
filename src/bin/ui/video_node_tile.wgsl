// A shader for drawing the video node preview
// (typically rendering a texture over top of a checkerboard to indicate transparency)

struct Uniforms {
    view: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) pos: vec2<f32>,
    @location(1) color: vec4<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
};

@vertex
fn vs_main(
    vertex: VertexInput,
) -> VertexOutput {

    var out: VertexOutput;
    out.position = uniforms.view * vec4<f32>(vertex.pos.x, vertex.pos.y, 0., 1.);
    out.color = vertex.color;
    return out;
}

// Fragment shader

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    return vertex.color;
}
