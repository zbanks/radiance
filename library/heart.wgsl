#property description Pink heart

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    // heart from shadertoy
    let normCoord = (uv - 0.5) * aspectCorrection + vec2<f32>(0., 0.15);
    let parameter = iIntensity * (1. + 0.5 * pow(defaultPulse, 2.));
    let normCoord = normCoord * (2. / parameter);
    let a = atan2(normCoord.x, -normCoord.y) / pi;
    let r = length(normCoord);
    let h = abs(a);
    let d = (13.0*h - 22.0*h*h + 10.0*h*h*h) / (6.0 - 5.0 * h);

    let c = vec4<f32>(1., 0.5, 0.5, 1.) * (1. - smoothstep(0., 3. * length(onePixel), r - d));
    return composite(fragColor, c);
}
