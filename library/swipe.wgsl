#property description Only update a vertical slice that slides across
#property frequency 1

// TODO this effect is stupid at frequency=0
// This effect doesn't really use iIntensity, I don't know how it's supposed to work

fn main(uv: vec2<f32>) -> vec4<f32> {
    let prev = textureSample(iChannelsTex[0], iSampler,  uv);
    let next = textureSample(iInputsTex[0], iSampler,  uv);
    let factor = smoothstep(0., 0.2, iIntensity);

    let t = fract(iTime * iFrequency * 0.25 - uv.x);
    let w = 1. - iIntensity;
    let x = select(0., pow(0.5 - t, 3.0), t < 0.5);

    let factor = min(factor, 1.0 - x);
    let factor = clamp(factor, 0.0, 1.0);

    return mix(next, prev, factor);
}
