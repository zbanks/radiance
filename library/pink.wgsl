#property description Pink polka dots

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    let r = 0.2;

    let normCoord = (uv - 0.5) * aspectCorrection;

    let count = 5.;
    let oneNormPixel = onePixel.x * aspectCorrection.x;
    let smooth_amt = oneNormPixel * count;

    let parameter = iIntensity * (0.7 + 0.3 * pow(defaultPulse, 2.));

    let c = vec4<f32>(1., 0.5, 0.5, 1.0);
    let c = c * (1. - smoothstep(r - smooth_amt, r, length((count + normCoord * 5. * parameter + 0.5) % vec2<f32>(1.) - 0.5)));
    let c = c * smoothstep(0., 0.1, iIntensity);
    return composite(fragColor, c);
}
