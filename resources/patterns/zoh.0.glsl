void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iFrame, uv);

    float t = pow(2, round(6 * iIntensity - 4));
    float a = 1;

    if (iIntensity < 0.09)
        a = 0;
    else if (mod(iTime, t) < 0.1)
        a = 0;

    gl_FragColor = mix(next, prev, a);
}
