#property description Set the color in HSV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let fragColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(fragColor.rgb);
    let h = max((iIntensity - 0.1) / 0.9, 0.);
    let hsv = vec3<f32>(h, hsv.yz);
    let rgb = mix(fragColor.rgb, hsv2rgb(hsv), smoothstep(0., 0.1, iIntensity) * defaultPulse);
    return add_alpha(rgb, fragColor.a);
}
