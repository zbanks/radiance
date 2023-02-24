#property description Basic white fill or strobe

fn main(uv: vec2<f32>) -> vec4<f32> {
    let pulse = pow(defaultPulse, 2.);
    let white = vec4<f32>(1.) * iIntensity * pulse;
    return composite(textureSample(iInputsTex[0], iSampler, uv), white);
}
