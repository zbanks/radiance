struct VertexOutput {
    @builtin(position) gl_Position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var varray = array<vec2<f32>, 4>(
        vec2<f32>(1., 1.),
        vec2<f32>(-1., 1.),
        vec2<f32>(1., -1.),
        vec2<f32>(-1., -1.),
    );

    let vertex = varray[vertex_index];
    let gl_Position = vec4<f32>(vertex, 0., 1.);
    let uv = 0.5 * (vertex + 1.);
    return VertexOutput(
        gl_Position,
        uv,
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

    return main(vertex.uv);
}
