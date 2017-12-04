#property description Smooth output

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iInput, uv);
    fragColor = mix(next, prev, pow(iIntensity, 0.4));
}
