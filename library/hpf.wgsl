#property description Smooth output, or first order (expontential) hold to the beat

fn main(uv: vec2<f32>) -> vec4<f32> {
    let cur = textureSample(iInputsTex[0], iSampler,  uv);
    let lpf = textureSample(iChannelsTex[1], iSampler,  uv);

    return mix(cur, abs(cur - lpf), smoothstep(0., 0.2, iIntensity));
}

#buffershader
fn main(uv: vec2<f32>) -> vec4<f32> {
    let cur = textureSample(iInputsTex[0], iSampler,  uv);
    let prev = textureSample(iChannelsTex[1], iSampler,  uv);

    let a = pow(mix(0.9, 0.6, iIntensity), 0.4);
    return mix(cur, prev, a);
}
