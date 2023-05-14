struct Uniforms {
    // Intensity slider, [0.0, 1.0]
    iIntensity: f32,
}

@group(0) @binding(0)
var<uniform> global: Uniforms;

@group(0) @binding(1)
var iSampler: sampler;

@group(0) @binding(2)
var iInputTex: texture_2d<f32>;

@group(0) @binding(3)
var iImageTex: texture_2d<f32>;

// Alpha-compsite two colors, putting one on top of the other. Everything is premultipled
fn composite(under: vec4<f32>, over: vec4<f32>) -> vec4<f32> {
    let a_out: f32 = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4<f32>((over.rgb + under.rgb * (1. - over.a)), a_out), vec4<f32>(0.), vec4<f32>(1.));
}

struct VertexOutput {
    @builtin(position) gl_Position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var pos_array = array<vec2<f32>, 4>(
        vec2<f32>(1., 1.),
        vec2<f32>(-1., 1.),
        vec2<f32>(1., -1.),
        vec2<f32>(-1., -1.),
    );
    var uv_array = array<vec2<f32>, 4>(
        vec2<f32>(1., 0.),
        vec2<f32>(0., 0.),
        vec2<f32>(1., 1.),
        vec2<f32>(0., 1.),
    );

    return VertexOutput(
        vec4<f32>(pos_array[vertex_index], 0., 1.),
        uv_array[vertex_index],
    );
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    let bg = textureSample(iInputTex, iSampler, vertex.uv);
    let movie_uv = vec2<f32>(vertex.uv.x, 1. - vertex.uv.y);
    let fg = textureSample(iImageTex, iSampler, movie_uv);
    return composite(bg, fg * global.iIntensity);
}
