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
    iAudio = global.iAudio;
    iTime = global.iTime;
    iFrequency = global.iFrequency;
    iIntensity = global.iIntensity;
    iIntensityIntegral = global.iIntensityIntegral;
    iResolution = global.iResolution;
    iStep = global.iStep;

    aspectCorrection = iResolution / min(iResolution.x, iResolution.y);
    defaultPulse = sawtooth(iTime * iFrequency, 0.1);

    return main(vertex.uv);
}
