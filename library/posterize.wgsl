#property description Reduce number of colors

fn main(uv: vec2<f32>) -> vec4<f32> {
    //float bins = 256. * pow(2, -8. * iIntensity);
    let bins = min(256., 1. / iIntensity);

    // bin in non-premultiplied space, then re-premultiply
    let c = demultiply(textureSample(iInputsTex[0], iSampler,  uv));
    let c = round(c * bins) / bins;
    let c = clamp(c, vec4<f32>(0.0), vec4<f32>(1.0));
    let fragColor = premultiply(c);
    return mix(textureSample(iInputsTex[0], iSampler,  uv), fragColor, pow(defaultPulse, 2.));
}
