#property description De-sync the shutter and the film feed

fn main(uv: vec2<f32>) -> vec4<f32> {

    // Lookup what offset to display the picture at
    let offset = textureSample(iChannelsTex[1], iSampler,  vec2<f32>(0.5, 0.5)).r;

    // Display the frame from iChannelsTex[2] at that offset
    return textureSample(iChannelsTex[2], iSampler,  (uv + vec2<f32>(0., offset)) % 1.);
}

#buffershader

// This shader integrates iIntensity
// on a per-frame basis
// to maintain vsync
// (all pixels are the same color)

fn main(uv: vec2<f32>) -> vec4<f32> {
    let parameter = iIntensity * sawtooth(iTime * iFrequency * 0.25, 0.5);
    let v = (textureSample(iChannelsTex[1], iSampler,  vec2<f32>(0.5, 0.5)).r + 1. - parameter) % 1.;

    // If intensity is low, decay to zero
    let v = max(0., v - 0.02 * (1. - step(0.03, iIntensity)));

    return vec4<f32>(v);
}

#buffershader

// This shader mirrors the input when intensity is low,
// and freezes the frame more and more as the intensity increases

fn main(uv: vec2<f32>) -> vec4<f32> {
    let freeze = step(0.8, iIntensity);
    return mix(textureSample(iInputsTex[0], iSampler,  uv), textureSample(iChannelsTex[2], iSampler,  uv), freeze);
}
