#property description Radiate color from the center based on audio
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let c = textureSample(iChannelsTex[1], iSampler,  uv);
    let c = c * (smoothstep(0., 0.2, iIntensity));
    return composite(c, fragColor);
}
#buffershader
fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iChannelsTex[1], iSampler,  (uv - 0.5) * 0.98 + 0.5);
    let fragColor = fragColor * (exp((iIntensity - 2.) / 50.));
    let fragColor = max(fragColor - 0.00001, vec4<f32>(0.));

    let c = textureSample(iInputsTex[0], iSampler,  uv) * pow(defaultPulse, 2.);
    return max(fragColor, c);
}
