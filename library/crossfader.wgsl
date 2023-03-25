#property description Mix between the two inputs
#property inputCount 2
fn main(uv: vec2<f32>) -> vec4<f32> {
    let l = textureSample(iInputsTex[0], iSampler, uv);
    let r = textureSample(iInputsTex[1], iSampler, uv);
    return mix(l, r, iIntensity * pow(defaultPulse, 2.));
}
