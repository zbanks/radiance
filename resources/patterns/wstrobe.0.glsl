void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.25) freq = 4.;
    else if(iIntensity < 0.45) freq = 2.;
    else if(iIntensity < 0.65) freq = 1.;
    else if(iIntensity < 0.85) freq = 0.5;
    else freq = 0.25;

    if(freq > 0) {
        c = vec4(1., 1., 1., sawtooth(iTime / freq, 0.1));
        gl_FragColor = composite(gl_FragColor, c);
    }
}
