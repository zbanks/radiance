#property description Invert the image colors

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor.rgb = mix(fragColor.rgb, fragColor.a - fragColor.rgb, iIntensity);
}
