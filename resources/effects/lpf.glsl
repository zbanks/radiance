// Smooth output

void main(void) {
    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iInput, uv);
    prev *= 0.98;
    gl_FragColor = mix(next, prev, pow(iIntensity, 0.4));
}
