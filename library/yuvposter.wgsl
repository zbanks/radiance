#property description Reduce number of colors in YUV space (but keep luminance)

fn main(uv: vec2<f32>) -> vec4<f32> {
    //float bins = 256. * pow(2, -8. * iIntensity);
    let bins = min(256., 1. / iIntensity);
    
    // bin in non-premultiplied space, then re-premultiply
    let oc = textureSample(iInputsTex[0], iSampler,  uv);
    let c = demultiply(oc);
    let rgb = rgb2yuv(c.rgb);
    let gb = clamp(round(rgb.gb * bins) / bins, vec2<f32>(0.0), vec2<f32>(1.0));
    let rgb = yuv2rgb(vec3<f32>(rgb.r, gb));
    let c = vec4<f32>(rgb, 1.) * c.a;
    return mix(oc, c, pow(defaultPulse, 2.));
}
