struct Uniforms {
    resolution: vec2<f32>,
    size: vec2<f32>,
}

@group(0) @binding(0)
var<uniform> global: Uniforms;

@group(0) @binding(1)
var iSampler: sampler;

@group(0) @binding(2)
var iWaveformTex: texture_1d<f32>;

@group(0) @binding(3)
var iBeatTex: texture_1d<f32>;

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
    let levelColor = vec4<f32>(0.667, 0.667, 0.667, 1.);
    let lowColor = vec4<f32>(0.267, 0., 0.444, 1.);
    let midColor = vec4<f32>(0.4, 0., 0.667, 1.);
    let highColor = vec4<f32>(0.667, 0., 1., 1.);

    let oneYPixel = 1. / global.resolution.y;
    let oneYPoint = 1. / global.size.y;

    let audio = textureSample(iWaveformTex, iSampler, 1. - vertex.uv.x);
    let beat1 = textureSample(iBeatTex, iSampler, 1. - vertex.uv.x).x;

    let wfDist1 = audio - abs(vertex.uv.y - 0.5) * 2.;
    let wfDist2 = wfDist1 + vec4<f32>(0., 0., 0., oneYPoint * 2.);
    let wf = smoothstep(vec4<f32>(0.), vec4<f32>(oneYPixel), wfDist2);

    let beat2 = beat1 * max(max(wf.x, wf.y), wf.z);

    let c1 = levelColor * wf.w;
    let c2 = composite(c1, lowColor * wf.x);
    let c3 = composite(c2, midColor * wf.y);
    let c4 = composite(c3, highColor * wf.z);
    let c5 = composite(c4, vec4(0., 0., 0., 0.5 * beat2));

    return c5;
}
