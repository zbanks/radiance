void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float xpos = iIntensityIntegral * 1.5;
    float xfreq = (iIntensity + 0.5) * 2.;
    float x = mod(uv.x * xfreq + xpos, 1.);
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor = composite(gl_FragColor, vec4(1., 1., 1., step(x, 0.3) * smoothstep(0., 0.5, iIntensity)));
}
