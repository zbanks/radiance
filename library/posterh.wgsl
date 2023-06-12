#property description Reduce number of hues in HSV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let parameter = iIntensity * (0.5 + 0.5 * pow(defaultPulse, 2.));
    let bins = min(360., 6. / parameter);

    let rgba = demultiply(textureSample(iInputsTex[0], iSampler,  uv));
    let hsv = rgb2hsv(rgba.rgb);
    let h = (round(hsv.r * bins) / bins) % 1.0;
    let hsv = vec3<f32>(h, hsv.yz);
    let rgb = hsv2rgb(hsv);

    return premultiply(vec4<f32>(rgb, rgba.a));
}
