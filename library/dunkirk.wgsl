#property description Apply a Dunkirk-esque (dark blue) palette

fn main(uv: vec2<f32>) -> vec4<f32> {
    let pulse = pow(defaultPulse, 2.);
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let h = hsv.x;
    let h = (h - 1. / 12.) % 1. - 6. / 12.;
    let h = h * (1. - iIntensity * 0.7);
    let h = (h + 7. / 12.) % 1.;
    let s = mix(hsv.y, 0., iIntensity * pulse * 0.4);
    let v = mix(hsv.z, 0., iIntensity * pulse * 0.3);
    let rgb = hsv2rgb(vec3<f32>(h, s, v));
    return vec4<f32>(rgb, max(max(max(fragColor.a, rgb.r), rgb.g), rgb.b));
}
