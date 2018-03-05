#property description Apply smoothing over time with new hits happening instantly

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iInput, uv);
    prev *= pow(iIntensity, 0.1);
    fragColor = max(prev, next * pow(defaultPulse, 2.));
}
