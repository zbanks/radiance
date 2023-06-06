#property description Distort the screen

fn main(uv: vec2<f32>) -> vec4<f32> {
    let noise_input = vec3<f32>(uv, iTime * 0.3);
    let shift = vec2<f32>(noise3(noise_input), noise3(noise_input + 100.)) - 0.5;
    let shift = shift + (vec2<f32>(noise3(2. * noise_input), noise3(2. * noise_input + 100.)) - 0.5) * 0.5;
    let shift = shift + (vec2<f32>(noise3(4. * noise_input), noise3(4. * noise_input + 100.)) - 0.5) * 0.25;
    let shift = 0.3 * shift;
    let shift = shift / aspectCorrection;

    let newUV = uv + shift * iIntensity * pow(defaultPulse, 2.) * 5.;

    return textureSample(iInputsTex[0], iSampler,  newUV) * box(newUV);
}
