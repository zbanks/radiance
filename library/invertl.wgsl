#property description Invert the image lightness, preserving color

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let hsv = vec3<f32>(hsv.xy, fragColor.a - hsv.z);
    let rgb = mix(fragColor.rgb, hsv2rgb(hsv), iIntensity * pow(defaultPulse, 2.));
    let a = max(max(max(fragColor.a, rgb.r), rgb.g), rgb.b);
    return vec4<f32>(rgb, a);
}
