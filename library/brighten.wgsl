#property description Make the image more white

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);
    return mix(c, vec4(c.a), iIntensity * pow(defaultPulse, 2.));
}
