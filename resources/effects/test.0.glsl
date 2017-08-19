// A green & red circle in the center

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;

    c = vec4(1., 1., 1., 1. - smoothstep(iIntensity - 0.1, iIntensity, length(normCoord)));
    c = premultiply(c);
    gl_FragColor = composite(gl_FragColor, c);

    c = texture2D(iChannel[1], (uv - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(normCoord));
    c = premultiply(c);
    gl_FragColor = composite(gl_FragColor, c);
}
