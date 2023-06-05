#property description Fileball in the center

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    let normCoord = (uv - 0.5) * aspectCorrection;

    let noise_input = vec3<f32>(length(normCoord) * 3. - iTime, abs(atan2(normCoord.y, normCoord.x)), iTime * 0.3);
    let shift = vec2<f32>(noise3(noise_input), noise3(noise_input + 100.)) - 0.5;
    let shift = shift + (vec2<f32>(noise3(2. * noise_input), noise3(2. * noise_input + 100.)) - 0.5) * 0.5;
    let shift = shift + (vec2<f32>(noise3(4. * noise_input), noise3(4. * noise_input + 100.)) - 0.5) * 0.25;
    let shift = (iIntensity * 0.7 + 0.3) * shift * (0.3 + 0.7 * pow(defaultPulse, 2.));

    let normCoord = normCoord + shift;
    let color = vec4<f32>(1., clamp(length(normCoord) * 2., 0., 1.), 0., 1.0);
    let color = color * smoothstep(0.4, 0.5, (1. - length(normCoord)));
    let color = color * smoothstep(0., 0.2, iIntensity);

    return composite(fragColor, color);
}
