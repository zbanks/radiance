// A simple shader for drawing the base video node tile
// (essentially some solid color polygons)

struct Uniforms {
    matrix: mat4x4<f32>,
};

struct VertexInput {
    @location(0) pos: vec2<f32>,
    @location(1) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(pos) pos: vec4<f32>,
    @location(0) color: vec4<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.color = in.color;
    out.pos = vec3<f32>(in.pos, 1.0);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> vec4<f32> {
    return in.color;
}
