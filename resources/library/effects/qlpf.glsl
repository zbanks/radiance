#property description Smooth output (quadratic LPF)

void main(void) {
    vec4 prev = texture(iChannel[0], uv);
    vec4 next = texture(iInput, uv);
    float d = distance(prev, next) / 2.0;
    float k = pow(iIntensity, 0.3) * (1.0 - pow(d, mix(2.5, 1.0, iIntensity)));
    fragColor = mix(next, prev, k);
}
