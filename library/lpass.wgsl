#property description Pass-thru when there are "lows" in the music

fn main(uv: vec2<f32>) -> vec4<f32> {
    let k = iAudioLow * 2.;
    return textureSample(iInputsTex[0], iSampler,  uv) * mix(1., min(k, 1.), iIntensity) * pow(defaultPulse, 2.);
}
