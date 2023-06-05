#property description Zero out the everything but the green channel (green is not a creative color)

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let parameter = iIntensity * pow(defaultPulse, 2.);
    let multiplier = vec4<f32>(1. - parameter, 1., 1. - parameter, 1.);
    return fragColor * multiplier;
}
