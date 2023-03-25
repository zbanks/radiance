#property description Overlay the second input on top of the first
#property inputCount 2

fn main(uv: vec2<f32>) -> vec4<f32> {
    let l = textureSample(iInputsTex[0], iSampler, uv);
    let r = textureSample(iInputsTex[1], iSampler, uv);
    return composite(l, r * iIntensity * pow(defaultPulse, 2.));
}
