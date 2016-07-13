void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.45) freq = 2.;
    else freq = 1.;

    if(freq > 0) {
        gl_FragColor.a *= 1. - ((1. - sawtooth(iTime / freq, 0.2)) * iIntensity* min(3. * iAudioLow, 1.));
    }
}
