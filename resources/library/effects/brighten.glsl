#property description Make the image more white

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor = mix(fragColor, vec4(fragColor.a), iIntensity * pow(defaultPulse, 2.));
}
