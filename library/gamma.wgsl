#property description Brighten the image using gamma correction

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    return pow(fragColor, vec4<f32>(1. / (1. + iIntensity * 3. * pow(defaultPulse, 2.))));
}

