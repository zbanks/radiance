#property description Shift the color in HSV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let h = (hsv.x + iIntensity) % 1.0;
    let rgb = hsv2rgb(vec3<f32>(h, hsv.yz));
    let c = vec4<f32>(rgb, max(max(max(fragColor.a, rgb.r), rgb.g), rgb.b));
    return mix(fragColor, c, pow(defaultPulse, 2.));
}
