#property description Deinterlacing artifacts

void main(void) {
    fragColor = texture(iInput, uv);

    float oddEven = mod(floor(uv.x * iResolution.x), 2.0) - 0.5; // either 0 or 1

    float pulse = pow(defaultPulse, 2.);

    vec2 offset = vec2(0.);
    offset.x += oddEven * 0.09 * smoothstep(0.0, 0.5, iIntensity);
    offset.x += oddEven * 0.04 * smoothstep(0.2, 0.6, iIntensity) * rand(vec2(iTime, uv.y / 1000.)) * pulse;
    offset.x += oddEven * 0.03 * smoothstep(0.4, 0.9, iIntensity) * pulse;
    offset.y += oddEven * 0.05 * smoothstep(0.8, 1.0, iIntensity);

    vec4 offColor = texture(iInput, clamp(uv + offset, 0., 1.));
    fragColor = mix(fragColor, offColor, iIntensity);
}
