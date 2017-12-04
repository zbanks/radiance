#property description Deinterlacing artifacts

void main(void) {
    fragColor = texture(iInput, uv);

    float oddEven = mod(floor(uv.x * iResolution.x), 2.0); // either 0 or 1

    vec2 offset = vec2(0);
    offset.x += 0.09 * smoothstep(0.0, 0.5, iIntensity);
    offset.x += 0.04 * smoothstep(0.2, 0.6, iIntensity) * rand(vec2(iTime, uv.y / 1000.));
    offset.x += 0.03 * smoothstep(0.4, 0.9, iIntensity) * sawtooth(iIntensityIntegral, 0.1);
    offset.y += 0.05 * smoothstep(0.8, 1.0, iIntensity);

    vec4 offColor = texture(iInput, clamp(uv + offset, 0, 1));
    fragColor = mix(fragColor, offColor, oddEven * iIntensity);
}
