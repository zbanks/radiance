#property description CRT-style effect

fn main(uv: vec2<f32>) -> vec4<f32> {
    let pulse = pow(defaultPulse, 0.5);
    let separate = iIntensity * vec2(0.005, 0.0) * pulse;
    let normCoord = (uv - 0.5) * aspectCorrection;
    let redOffset = normCoord - separate;
    let greenOffset = normCoord;
    let blueOffset = normCoord + separate;

    let redImage = textureSample(iInputsTex[0], iSampler, redOffset / aspectCorrection + 0.5);
    let greenImage = textureSample(iInputsTex[0], iSampler, greenOffset / aspectCorrection + 0.5);
    let blueImage = textureSample(iInputsTex[0], iSampler, blueOffset / aspectCorrection + 0.5);

    let rgb = vec3(redImage.r, greenImage.g, blueImage.b);
    let rgb = rgb * mix(1.0, 1.0 - pow(abs(sin(greenOffset.y * 160.0)), 16.), iIntensity * pulse / 3.0);
    let r = rgb.r * mix(1.0, 1.0 - pow(abs(sin(redOffset.x * 200.0)), 6.), iIntensity * pulse);
    let g = rgb.g * mix(1.0, 1.0 - pow(abs(sin(greenOffset.x * 200.0)), 6.), iIntensity * pulse);
    let b = rgb.b * mix(1.0, 1.0 - pow(abs(sin(blueOffset.x * 200.0)), 6.), iIntensity * pulse);
    let a_out = max(max(greenImage.a, r), max(g, b));
    return vec4(r, g, b, a_out);
}
