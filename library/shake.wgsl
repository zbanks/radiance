#property description Shake the image when there are lows

fn main(uv: vec2<f32>) -> vec4<f32> {
    let t = iTime * 2. *  pi;
    let sweep = vec2<f32>(cos(3. * t), sin(2. * t));

    let amount = iIntensity * 0.3 * max(iAudioLow - 0.2, 0.) * sawtooth(iTime * iFrequency, 0.3);

    let newUV = (uv - 0.5) + sweep * amount + 0.5;

    let fragColor = textureSample(iInputsTex[0], iSampler,  newUV) * box(newUV);
    return fragColor;
}
