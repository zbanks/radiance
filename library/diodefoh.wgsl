#property description First order (expontential) hold to the beat, but diode

fn main(uv: vec2<f32>) -> vec4<f32> {
    let prev = textureSampleLevel(iChannelsTex[0], iSampler,  uv, 0.);
    let next = textureSampleLevel(iChannelsTex[1], iSampler,  uv, 0.);

    let smoothed = mix(next, prev, pow(iIntensity, 0.4));
    let sharp = next;
    let fragColor = select(sharp, smoothed, next.a > prev.a);
    let fragColor = clamp(fragColor, vec4<f32>(0.), vec4<f32>(1.));
    return fragColor;
}

#buffershader
fn main(uv: vec2<f32>) -> vec4<f32> {
    let t = pow(2., round(6. * iIntensity - 4.));

    let refresh_sample = textureSampleLevel(iInputsTex[0], iSampler,  uv, 0.);
    let hold_sample = textureSampleLevel(iChannelsTex[1], iSampler,  uv, 0.);

    let fragColor = select(hold_sample, refresh_sample, iIntensity < 0.09 || iTime % t < 0.1);
    return fragColor;
}
