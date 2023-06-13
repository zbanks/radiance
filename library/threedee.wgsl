#property description Fake 3D-glasses effect

fn main(uv: vec2<f32>) -> vec4<f32> {
    let baseImage = textureSample(iInputsTex[0], iSampler,  uv);
    let sep = iIntensity * ((baseImage.r + baseImage.g + baseImage.b) * 0.2 + 0.4) * 0.05;
    let sep = sep * (pow(defaultPulse, 0.5));

    let redImage = textureSample(iInputsTex[0], iSampler,  uv + vec2<f32>(sep, 0.));
    let blueImage = textureSample(iInputsTex[0], iSampler,  uv - vec2<f32>(sep, 0.));

    let r = mix(baseImage.r, redImage.r / redImage.a * baseImage.a, 0.9);
    let g = mix(baseImage.g, blueImage.g / blueImage.a * baseImage.a, 0.3);
    let b = mix(baseImage.b, blueImage.b / blueImage.a * baseImage.a, 0.6);
    return add_alpha(vec3<f32>(r, g, b), baseImage.a);
}
