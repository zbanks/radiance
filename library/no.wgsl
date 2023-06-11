#property description Reduce alpha (make input go away) or inverse-strobe

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return fragColor * ((1. - iIntensity * pow(defaultPulse, 2.)));
}
