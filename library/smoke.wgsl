#property description Perlin noise green smoke

// Something's wrong with noise3, it doesn't look good

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;

    let noise_input = vec3<f32>(normCoord * iIntensity * 4., iIntensity + iIntensityIntegral * iFrequency);
    let n = noise3(noise_input) - 0.1;
    let n = n + ((noise3(2. * noise_input) - 0.5) * 0.5);
    let n = n + ((noise3(4. * noise_input) - 0.5) * 0.25);
    let n = n + ((noise3(8. * noise_input) - 0.5) * 0.125);
    let n = n + ((noise3(16. * noise_input) - 0.5) * 0.0625);
    let n = n / 3.;

    let a = clamp(n * n * 5., 0., 1.) * smoothstep(0., 0.2, iIntensity);

    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return composite(fragColor, vec4<f32>(0., a, 0., a));
}
