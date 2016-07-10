void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    float SMOOTH = 0.01;

    float lows = 1. - smoothstep(iAudioLow, iAudioLow + SMOOTH, abs(uv.x - 0.5));
    float mids = 1. - smoothstep(iAudioMid, iAudioMid + SMOOTH, abs(uv.x - 0.5));
    float his  = 1. - smoothstep(iAudioHi , iAudioHi  + SMOOTH, abs(uv.x - 0.5));

    vec4 c = composite(composite(vec4(0., 0., 0.5, lows), vec4(0., 0., 1., mids)), vec4(0.3, 0.3, 1., his));
    c.a = clamp(c.a, 0., 1.);
    c.a *= iIntensity;
    gl_FragColor = composite(gl_FragColor, c);
}
