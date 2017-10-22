// Strobe alpha to the beat

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c;

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.45) freq = 2.;
    else freq = 1.;

    if(freq > 0) {
        fragColor.a *= 1. - ((1. - sawtooth(iTime / freq, 0.2)) * iIntensity* min(3. * iAudioLevel, 1.));
    }
}
