#property description Cycle quickly through the rainbow according to lightness, giving things a rainbow sheen

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let phase = hsv.x + hsv.z * iIntensity * 30. * (1. - hsv.z) + iIntensityIntegral * iFrequency;
    let h = fract(phase);
    let rgb = hsv2rgb(vec3<f32>(h, hsv.yz));
    return add_alpha(rgb, fragColor.a);
}
