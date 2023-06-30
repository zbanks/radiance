struct Uniforms {
    factor: vec2<f32>,
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
    @location(1) image_uv: vec2<f32>,
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

    let uv = uv_array[vertex_index];
    let image_uv = (uv - 0.5) * global.factor + 0.5;

    return VertexOutput(
        vec4<f32>(pos_array[vertex_index], 0., 1.),
        uv,
        image_uv,
    );
}

fn box(p: vec2<f32>) -> f32 {
    let b = step(vec2<f32>(0.), p) - step(vec2<f32>(1.), p);
    return b.x * b.y;
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    let bg = textureSample(iInputTex, iSampler, vertex.uv);
    let fg = textureSample(iImageTex, iSampler, vertex.image_uv) * box(vertex.image_uv);
    return composite(bg, fg);
}
