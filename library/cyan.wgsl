#property description Cyan diagonal stripes
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let o = textureSample(iInputsTex[0], iSampler, uv);

    let normCoord = (uv - 0.5) * aspectCorrection;

    let t = normCoord.x * 3.0 + normCoord.y * 3.0;
    let y = smoothstep(0.2, 0.7, abs(modulo(t - 3. * iIntensityIntegral * iFrequency, 2.) - 1.));
    let g = smoothstep(0.5, 0.9, abs(modulo(1. + t - 3. * iIntensityIntegral * iFrequency, 2.) - 1.));

    let c = vec4(0., 1., 1., 1.) * y;
    let d = vec4(0., 0., 1., 1.) * g * smoothstep(0.5, 0.8, iIntensity);
    let c = composite(c, d);

    let c = c * smoothstep(0., 0.1, iIntensity);
    let c = clamp(c, vec4<f32>(0.), vec4<f32>(1.));
    return composite(o, c);
}
