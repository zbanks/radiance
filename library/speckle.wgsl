#property description Per-pixel twinkle effect

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iChannelsTex[0], iSampler,  uv);
    var fragColor = fragColor * (exp(-iIntensity / 20.));
    if (rand3(vec3<f32>(uv, iTime)) < exp(-iIntensity * 3.) * sawtooth(iTime * iFrequency, 0.9)) {
        fragColor = textureSampleLevel(iInputsTex[0], iSampler,  uv, 1.);
    }
    return fragColor;
}
