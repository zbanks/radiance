void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    gl_FragColor = texture2D(iChannel[1], (uv - 0.5) * 0.98 + 0.5);
    gl_FragColor.a *= exp((iIntensity - 2.) / 50.) * smoothstep(0, 0.01, length(uv - 0.5));

    vec4 c = texture2D(iFrame, uv);
    float s = smoothstep(0.90, 1., 1. - mod(iTime, 1.)) * iAudioLow;
    c.a *=  min(3. * s, 1.);
    gl_FragColor = composite(gl_FragColor, c);
}
