#property description Shift the hue on the beat
#property frequency 1

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    
    let t = iTime * iFrequency;
    let deviation = (2. * floor(t)) % 8. / 8.;
    let deviation = deviation * (clamp(iIntensity / 0.8, 0., 1.));

    let hsv = rgb2hsv(fragColor.rgb);
    let h = fract(hsv.r + 1. + deviation);
    let rgb = hsv2rgb(vec3<f32>(h, hsv.yz));
    return add_alpha(rgb, fragColor.a);
}
