#property description Push colors towards extremes with smoothstep

void main(void) {
    vec4 color = demultiply(texture(iInput, uv));

    float halfWidth = mix(0.5, 0.05, iIntensity);
    vec3 targetColor = smoothstep(0.5 - halfWidth, 0.5 + halfWidth, color.rgb);

    // Even when halfWidth = 0.5, smoothstep is not the identity
    // so we mix here to preserve keep the identity
    color.rgb = mix(color.rgb, targetColor, iIntensity);

    fragColor = premultiply(color);
}
