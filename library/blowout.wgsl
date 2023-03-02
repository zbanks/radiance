#property description Push colors towards extremes with smoothstep

fn main(uv: vec2<f32>) -> vec4<f32> {
    let color4 = demultiply(textureSample(iInputsTex[0], iSampler, uv));
    let color = color4.rgb;

    let halfWidth = mix(0.5, 0.05, iIntensity);
    let targetColor = smoothstep(vec3<f32>(0.5 - halfWidth), vec3<f32>(0.5 + halfWidth), color);

    // Even when halfWidth = 0.5, smoothstep is not the identity
    // so we mix here to preserve keep the identity
    let color = mix(color, targetColor, iIntensity * pow(defaultPulse, 2.));

    return premultiply(vec4<f32>(color, color4.a));
}
