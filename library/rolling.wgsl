#property description Rolling shutter effect
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let rate = 1.0 / (iFrequency + step(0.0, -iFrequency) * 0.125);
    let yv = (iTime % rate) / rate;
    let old = textureSample(iChannelsTex[0], iSampler,  uv);
    let new_ = textureSample(iInputsTex[0], iSampler,  uv);
    let dist = abs(uv.y - yv); // TODO: make this wrap around
    let fragColor = mix(old, new_, max(0., 1.0 - (1. / iStep) * (0.1 * rate) * dist));
    let fragColor = mix(new_, fragColor, pow(iIntensity, 0.1));
    return fragColor;
}
