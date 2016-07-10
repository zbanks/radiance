void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    c = vec4(1., 1., 1., 1 - smoothstep(iIntensity - 0.1, iIntensity, length(uv - 0.5) / 0.5));
    gl_FragColor = composite(gl_FragColor, c);

    c = texture2D(iChannel[1], (uv - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(uv - 0.5) / 0.5);
    gl_FragColor = composite(gl_FragColor, c);
}
