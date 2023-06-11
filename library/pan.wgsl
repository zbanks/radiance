#property description Move input horizontally

fn main(uv: vec2<f32>) -> vec4<f32> {
    let x = (uv.x + iIntensity * pow(defaultPulse, 2.)) % 1.;
    let y = uv.y;
    return textureSample(iInputsTex[0], iSampler,  vec2<f32>(x, y));
}
