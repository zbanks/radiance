//#property description Zoom in, or bounce to the beat

fn main(uv: vec2<f32>) -> vec4<f32> {
    let factor = 1. - iIntensity * pow(defaultPulse, 2.);
    let factor = clamp(factor, 0.05, 2.);

    return textureSample(iInputsTex[0], iSampler, (uv - 0.5) * factor + 0.5);
}
