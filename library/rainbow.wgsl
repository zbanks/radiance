#property description Cycle the color (in HSV) over time
#property frequency 0.5

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);

    let deviation = select(iIntensity, fract(iFrequency * iTime * 0.5), iFrequency != 0.);

    let hsv = rgb2hsv(fragColor.rgb);
    let h = fract(hsv.r + 1. + deviation);
    let rgb = hsv2rgb(vec3<f32>(h, hsv.yz));
    let c = add_alpha(rgb, fragColor.a);

    let fragColor = mix(fragColor, c, iIntensity);
    return select(c, mix(fragColor, c, iIntensity), iFrequency != 0.);
}
