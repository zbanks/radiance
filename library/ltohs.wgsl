#property description Set the hue and saturation equal to the lightness in HSV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let parameter = iIntensity * pow(defaultPulse, 0.5);
    let h = mix(hsv.x, hsv.z, smoothstep(0., 0.5, parameter));
    let s = mix(hsv.y, hsv.z, smoothstep(0.5, 1.0, parameter));
    let v = hsv.z;
    let rgb = hsv2rgb(vec3<f32>(h, s, v));
    let a = max(max(max(fragColor.a, rgb.r), rgb.g), rgb.b);
    return vec4<f32>(rgb, a);
}
