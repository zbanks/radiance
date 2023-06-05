#property description Apply smoothing over time with new hits happening instantly

fn main(uv: vec2<f32>) -> vec4<f32> {
    let prev = textureSample(iChannelsTex[0], iSampler,  uv);
    let next = textureSample(iInputsTex[0], iSampler,  uv);
    let prev = prev * (pow(iIntensity, 0.1)) * (1. - pow(defaultPulse, 4.));
    return max(prev, next);
}
