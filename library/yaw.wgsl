#property description Move input vertically

fn main(uv: vec2<f32>) -> vec4<f32> {
    let u = fract(uv.x + iIntensity * pow(defaultPulse, 2.));
    let v = uv.y;
    return textureSample(iInputsTex[0], iSampler,  vec2<f32>(u, v));
}
