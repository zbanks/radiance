#property description Brightly highlight pixels in pink where rgb > a

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);
    let a = max(max(c.r, c.g), c.b);

    let pink = vec4<f32>(1., 0., 1., 1.);
    let highlight = select(c, pink, a > c.a);

    return mix(c, highlight, iIntensity * defaultPulse);
}
