#property description Solid rainbow colors

void main(void) {
    float h = clamp((iIntensity - 0.25) / 0.9, 0.0, 1.0);

    vec3 color = hsv2rgb(vec3(h, 1.0, 1.0));
    color = mix(vec3(0.0), color, smoothstep(0.10, 0.12, iIntensity));
    color = mix(color, vec3(1.0), smoothstep(0.90, 1.00, iIntensity));

    fragColor = texture(iInput, uv);
    fragColor = mix(fragColor, vec4(color, 1.0), smoothstep(0.0, 0.1, iIntensity));
}
