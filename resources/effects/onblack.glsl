#property description Composite the input image onto black

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor.rgb *= mix(1.0, fragColor.a, iIntensity);
    fragColor.a = mix(fragColor.a, 1.0, iIntensity);
}
