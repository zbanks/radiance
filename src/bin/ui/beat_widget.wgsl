struct Uniforms {
    resolution: vec2<f32>,
    size: vec2<f32>,
    beat: f32,
}

@group(0) @binding(0)
var<uniform> global: Uniforms;

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

// Box from [0, 0] to (1, 1)
fn box(p: vec2<f32>) -> f32 {
    let b = step(vec2<f32>(0.), p) - step(vec2<f32>(1.), p);
    return b.x * b.y;
}

@fragment
fn fs_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    let ballOutlineColor = vec4<f32>(0.267, 0., 0.444, 1.);
    let ballColorBottom = vec4<f32>(0.4, 0., 0.667, 1.);
    let ballColorTop = vec4<f32>(0.667, 0., 1., 1.);
    let floorColor = vec4<f32>(0.667, 0.667, 0.667, 1.);

    let height1 = 1. - pow(abs(2. * (fract(global.beat) - 0.5)), 2.);
    let height2 = height1 * (1. - 0.4 * (1. - step(3., global.beat % 4.)));
    let ballLoc = vec2<f32>(0.5, 0.8 - 0.6 * height2);
    let ballColor = mix(ballColorBottom, ballColorTop, clamp(10. * (ballLoc.y - vertex.uv.y), 0., 1.));
    let ball = 1. - smoothstep(0.08, 0.09, length(vertex.uv - ballLoc));
    let ballOutline = 1. - smoothstep(0.09, 0.1, length(vertex.uv - ballLoc));
    let floorBox = box((vertex.uv - vec2(0.2, 0.9)) / vec2(0.6, 0.05));

    let fragColor1 = floorBox * floorColor;
    let fragColor2 = composite(fragColor1, ballOutline * ballOutlineColor);
    let fragColor3 = composite(fragColor2, ball * ballColor);
    return fragColor3;
}
