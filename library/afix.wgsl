#property description Fix out-of-bounds values in premultiplied-alpha space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);
    let a = max(max(c.r, c.g), max(c.b, c.a));
    return vec4<f32>(c.rgb, mix(c.a, a, iIntensity * defaultPulse));
}
