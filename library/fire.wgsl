#property description Fire from the bottom

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    let normCoord = (uv - 0.5) * aspectCorrection;

    let noise_input = vec3<f32>(normCoord * 3. + vec2<f32>(0., iTime * 0.5), iTime * 0.25);
    let shift = (vec2<f32>(noise3(noise_input), noise3(noise_input + 100.)) - 0.5);
    let shift = shift + (vec2<f32>(noise3(2. * noise_input), noise3(2. * noise_input + 100.)) - 0.5) * 0.5;
    let shift = shift + (vec2<f32>(noise3(4. * noise_input), noise3(4. * noise_input + 100.)) - 0.5) * 0.25;
    let shift = (iIntensity * 0.5 + 0.5) * (0.7 * pow(defaultPulse, 2.) + 0.3) * shift + vec2<f32>(0., -0.5 + 0.5 * iIntensity);
    let shift = shift / aspectCorrection;

    let uv2 = uv + shift;
    let color = vec4<f32>(1., (1. - uv2.y) * 0.6, 0., 1.0);
    let color = color * smoothstep(0.1, 0.3, uv2.y);
    let color = color * smoothstep(0., 0.2, iIntensity);

    return composite(fragColor, color);
}
