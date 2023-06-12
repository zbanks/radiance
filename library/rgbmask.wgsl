#property description Use the R, G, B channels of the first input to mask the other 3 inputs
#property inputCount 4

fn main(uv: vec2<f32>) -> vec4<f32> {
    let m = textureSample(iInputsTex[0], iSampler,  uv);
    let r = textureSample(iInputsTex[1], iSampler,  uv);
    let g = textureSample(iInputsTex[2], iSampler,  uv);
    let b = textureSample(iInputsTex[3], iSampler,  uv);

    let f = m.a * iIntensity * defaultPulse;
    let fragColor = vec4<f32>(mix(m.rgb, vec3<f32>(0.0), f), m.a);

    let fragColor = composite(fragColor, r * m.r * f);
    let fragColor = composite(fragColor, g * m.g * f);
    let fragColor = composite(fragColor, b * m.b * f);
    return fragColor;
}
