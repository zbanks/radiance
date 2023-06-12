#property description That cool thing that happens when you beat solitaire
#property frequency 2

fn main(uv: vec2<f32>) -> vec4<f32> {
    // Relative size of the bouncing image
    // I don't think we need to encorporate aspectCorrection?
    let scale = mix(1.0, 0.25, smoothstep(0., 0.3, iIntensity));

    // Closed-form bouncing behavior
    // I think this is periodic over [0.0, 16.] to prevent discontinuities
    let phase = iTime * iFrequency / 32.;
    let xy = vec2<f32>(sawtooth(phase * 5.0, 0.5), abs(sin(phase * 9.0 * pi)));

    let xy = (xy - 0.5) * (1.0 - scale);
    let uvSample = (uv - 0.5 + xy) / scale + 0.5;

    let c = textureSample(iInputsTex[0], iSampler,  uvSample);
    //let c = vec4<f32>(c.rgb, mix(c.a, 1., iIntensity)); // onblack
    let c = c * box(uvSample);
    let under = textureSample(iChannelsTex[0], iSampler,  uv) * smoothstep(0., 0.1, iIntensity);
    return composite(under, c);
}
