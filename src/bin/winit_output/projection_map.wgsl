@group(0) @binding(0)
var iTexture: texture_2d<f32>;

struct Uniforms {
    // Mapping from physical UV space to virtual UV spac
    inv_map: mat3x3<f32>
}

struct Vertex {
    // Coordinate of the crop polygon in physical UV space
    @location(0) uv: vec2<f32>
}

@group(0) @binding(1)
var iSampler: sampler;

@group(0) @binding(2)
var<uniform> global: Uniforms;

struct VertexOutput {
    @builtin(position) gl_Position: vec4<f32>,
    @location(0) uv: vec3<f32>,
};

@vertex
fn vs_main(vertex: Vertex) -> VertexOutput {
    let pos = vec4<f32>(vertex.uv.x * 2. - 1., 1. - vertex.uv.y * 2., 0., 1.);
    let mapped_uv = global.inv_map * vec3<f32>(vertex.uv, 1.);

    return VertexOutput(
        pos,
        mapped_uv,
    );
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(iTexture, iSampler, vertex.uv.xy / vertex.uv.z);
}
