#property description Zoom out

fn main(uv: vec2<f32>) -> vec4<f32> {
    let factor = 1. - iIntensity * pow(defaultPulse, 2.);
    let factor = clamp(factor, 0.05, 2.);

    let texcoords = (uv - 0.5) / factor + 0.5;
    return textureSample(iInputsTex[0], iSampler,  texcoords) * box(texcoords);
}
