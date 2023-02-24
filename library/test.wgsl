//#property description A green & red circle in the center

fn main(uv: vec2<f32>) -> vec4<f32> {
    let out_c = textureSample(iInputsTex[0], iSampler, uv);

    let normCoord = 2. * (uv - 0.5) * aspectCorrection;
    let r = iIntensity * (0.7 + 0.3 * pow(defaultPulse, 2.));

    // White outline
    let c = vec4<f32>(1.);
    let c = c * (1. - smoothstep(r - 0.1, r, length(normCoord)));
    let out_c = composite(out_c, c);
    
    /// Red and green (sampled from buffer shader)
    let c = textureSample(iChannelsTex[1], iSampler, (uv - 0.5) / r + 0.5);
    let c = c * (1. - smoothstep(r - 0.2, r - 0.1, length(normCoord)));
    let out_c = composite(out_c, c);

    return out_c;
}

#buffershader

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = 2. * (uv - 0.5) * aspectCorrection;
    return vec4<f32>(abs(normCoord), 0., 1.);
}
