#property description Full black strobe
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let c = textureSample(iInputsTex[0], iSampler, uv);

    let darkness = select(smoothstep(0., 0.2, iIntensity), pow(iIntensity, 2.), iFrequency == 0.);
    let darkness = vec4(0., 0., 0., 1.) * pow(defaultPulse, (iIntensity + 0.5) * 2.) * darkness;

    return composite(c, darkness);
}
