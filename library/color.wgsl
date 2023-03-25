#property description Solid rainbow colors

fn main(uv: vec2<f32>) -> vec4<f32> {
    let h = clamp((iIntensity - 0.25) / 0.9, 0.0, 1.0);

    let color = hsv2rgb(vec3(h, 1.0, 1.0));
    let color = mix(vec3(0.0), color, smoothstep(0.10, 0.12, iIntensity));
    let color = mix(color, vec3(1.0), smoothstep(0.90, 1.00, iIntensity));

    let c = textureSample(iInputsTex[0], iSampler, uv);
    return mix(c, vec4(color, 1.0), smoothstep(0.0, 0.1, iIntensity));
}

