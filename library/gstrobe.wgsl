#property description Strobe green to the beat
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler,  uv);

    let i = pow(defaultPulse, 2.);
    let multiplier = vec4<f32>(1. - i, i, 1. - i, 1.);
    let fragColor = mix(c, c * multiplier, iIntensity);
    return fragColor;
}
