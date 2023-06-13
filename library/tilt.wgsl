#property description Move input vertically

fn main(uv: vec2<f32>) -> vec4<f32> {
    let u = uv.x;
    let v = fract(uv.y + iIntensity * pow(defaultPulse, 2.));
    return textureSample(iInputsTex[0], iSampler,  vec2<f32>(u, v));
}
