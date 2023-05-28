@group(0) @binding(0)
var iTexture: texture_2d<f32>;

struct Uniforms {
    // Mapping from physical UV space to virtual UV spac
    inv_map: mat3x3<f32>
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
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var pos_array = array<vec2<f32>, 4>(
        vec2<f32>(1., 1.),
        vec2<f32>(-1., 1.),
        vec2<f32>(1., -1.),
        vec2<f32>(-1., -1.),
    );
    var uv_array = array<vec3<f32>, 4>(
        vec3<f32>(1., 0., 1.),
        vec3<f32>(0., 0., 1.),
        vec3<f32>(1., 1., 1.),
        vec3<f32>(0., 1., 1.),
    );

    return VertexOutput(
        vec4<f32>(pos_array[vertex_index], 0., 1.),
        global.inv_map * uv_array[vertex_index],
    );
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(iTexture, iSampler, vertex.uv.xy / vertex.uv.z);
}
