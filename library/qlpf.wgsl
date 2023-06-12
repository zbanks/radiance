#property description Smooth output, or first order (expontential) hold to the beat

fn main(uv: vec2<f32>) -> vec4<f32> {
    let prev = textureSample(iChannelsTex[0], iSampler, uv);
    let next = textureSample(iChannelsTex[1], iSampler, uv);

    let d = distance(prev, next) / 2.0;
    let k = pow(iIntensity, 0.3) * (1.0 - pow(d, mix(2.5, 1.0, iIntensity)));

    return clamp(mix(next, prev, k), vec4<f32>(0.), vec4<f32>(1.));
}

#buffershader
fn main(uv: vec2<f32>) -> vec4<f32> {
    let transVec = textureSample(iChannelsTex[2], iSampler, vec2(0.));
    let trans = step(0.5, transVec.r - transVec.g) + step(0., -iFrequency);
    let a = 1. - (1. - trans) * smoothstep(0., 0.2, iIntensity);
    return mix(textureSample(iChannelsTex[1], iSampler, uv), textureSample(iInputsTex[0], iSampler, uv), a);
}

#buffershader
// This shader stores the last value of defaultPulse in the red channel
// and the current value in the green channel.

fn main(uv: vec2<f32>) -> vec4<f32> {
    let last = textureSample(iChannelsTex[2], iSampler, vec2<f32>(0.)).g;
    return vec4<f32>(last, (iFrequency * iTime) % 1., 0., 1.);
}
