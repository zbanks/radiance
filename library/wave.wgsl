#property description Green and blue base pattern

fn main(uv: vec2<f32>) -> vec4<f32> {
    let normCoord = (uv - 0.5) * aspectCorrection;
    let c = vec4<f32>(0., 0., 0., 1.);
    let t = iTime * iFrequency;
    let ratio = 5. * pi;
    let r = 0.0;
    let g = 0.5 * sin((normCoord.x + normCoord.y - t * 0.25) * ratio) + 0.5;
    let b = 0.5 * sin((normCoord.x - normCoord.y) * ratio) + 0.5;
    let a = 1.0;

    let c = vec4<f32>(r, g, b, a);

    return composite(textureSample(iInputsTex[0], iSampler,  uv), c * iIntensity);
}
