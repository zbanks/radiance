#property description Saturate colors in HSV space

fn main(uv: vec2<f32>) -> vec4<f32> {
    let origColor = textureSample(iInputsTex[0], iSampler,  uv);
    let hsv = rgb2hsv(demultiply(origColor).rgb);
    let s = mix(hsv.y, 1., iIntensity * defaultPulse);
    let rgb = hsv2rgb(vec3<f32>(hsv.x, s, hsv.z));
    return add_alpha_dimming(rgb, origColor.a);
}

