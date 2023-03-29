struct Uniforms {
    resolution: vec2<f32>,
    size: vec2<f32>,
}

@group(0) @binding(0)
var<uniform> global: Uniforms;

@group(0) @binding(1)
var iSampler: sampler;

@group(0) @binding(2)
var iSpectrumTex: texture_1d<f32>;

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

// Alpha-compsite two colors, putting one on top of the other
fn composite(under: vec4<f32>, over: vec4<f32>) -> vec4<f32> {
    let a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4<f32>((over.rgb + under.rgb * (1. - over.a)), a_out), vec4<f32>(0.), vec4<f32>(1.));
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    let spectrumColorOutline = vec4<f32>(0.667, 0.667, 0.667, 1.);
    let spectrumColorBottom = vec4<f32>(0.4, 0., 0.667, 1.);
    let spectrumColorTop = vec4<f32>(0.667, 0., 1., 1.);

    let freq = vertex.uv.x;
    let h = textureSample(iSpectrumTex, iSampler, freq).r;

    let smoothEdge = 0.04;
    let h = h * smoothstep(0., smoothEdge, freq) - smoothstep(1. - smoothEdge, 1., freq);
    let d = (vertex.uv.y - (1. - h)) * 90.; // TODO this 1 - h is weird
    let c = mix(spectrumColorTop, spectrumColorBottom, clamp(d / 30., 0., 1.)) * step(1., d);
    let c = composite(c, spectrumColorOutline * (smoothstep(0., 1., d) - smoothstep(3., 4., d) ));

    return c;
}
