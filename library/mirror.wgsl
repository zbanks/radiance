#property description Place a vertical mirror to make the frame symmetric

fn main(uv: vec2<f32>) -> vec4<f32> {
    let xPos = (1. - smoothstep(0., 0.2, iIntensity)) * 0.5 + 0.5;
    let x = -abs(uv.x - xPos) + xPos;
    let x = x + (iIntensity * 0.5 * pow(defaultPulse, 2.));
    return textureSample(iInputsTex[0], iSampler,  vec2<f32>(x, uv.y));
}
