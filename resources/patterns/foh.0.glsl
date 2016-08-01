// First order (expontential) hold

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec4 prev = texture2D(iChannel[0], uv);
    vec4 next = texture2D(iChannel[1], uv);

/*
    float t = pow(2, round(6 * iIntensity - 4));
    float a = 0.98;
    if (iIntensity < 0.09 || mod(iTime, t) < 0.1)
        gl_FragColor = next;
        */

    gl_FragColor = mix(next, prev, pow(iIntensity, 0.4));
    gl_FragColor = clamp(gl_FragColor, 0, 1);
}
