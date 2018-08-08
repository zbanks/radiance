#property description Transpose high & low bits of each RGB byte

void main(void) {
    fragColor = texture(iInput, uv);
    float v = iIntensity * defaultPulse;

    float nibble_pow2_bits = pow(16., v);

    vec3 highs;
    vec3 lows = modf(fragColor.rgb * nibble_pow2_bits, highs);
    fragColor.rgb = mix(
        fragColor.rgb,
        lows + highs / 256.,
        smoothstep(0., 0.2, v));
}
