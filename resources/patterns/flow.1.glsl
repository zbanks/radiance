void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    gl_FragColor = texture2D(iChannel[1], (uv - 0.5) * 0.98 + 0.5);
    gl_FragColor.a *= exp((iIntensity - 2.) / 50.);

    vec4 c = texture2D(iFrame, uv);
    float s = smoothstep(0.90, 1., 1. - mod(iTime, 1.));
    c.a *= s;
    gl_FragColor = composite(gl_FragColor, c);
}
