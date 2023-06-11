#property description Zero out the green channel (green is not a creative color)

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let g = fragColor.g * (1. - iIntensity * pow(defaultPulse, 2.));
    return vec4<f32>(fragColor.r, g, fragColor.b, fragColor.a);
}
