#property description Change the color to red

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler,  uv);
    let parameter = iIntensity * pow(defaultPulse, 2.);
    let r = mix(c.r, (c.r + c.g + c.b) / 3., parameter);
    let g = c.g * (1. - parameter);
    let b = c.b * (1. - parameter);
    return add_alpha(vec3<f32>(r, g, b), c.a);
}
