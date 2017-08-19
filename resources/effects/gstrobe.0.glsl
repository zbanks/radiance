// Strobe alpha to the beat

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec4 c;

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.45) freq = 2.;
    else freq = 1.;

    if(freq > 0) {
        float i = (1. - sawtooth(iTime / freq, 0.1)) * min(3. * iAudioLevel, 1.);
        gl_FragColor.r *= i;
        gl_FragColor.b *= i;
        gl_FragColor.g *= 1. - i;
    }
}
