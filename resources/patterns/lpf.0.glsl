// Smooth output

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iFrame, uv);
    prev.a *= 0.98;
    gl_FragColor = mix(next, prev, pow(iIntensity, 0.4));
}
