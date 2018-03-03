#property description Pass-thru when there are "lows" in the music

void main(void) {
    float k = iAudioLow * 2.;
    fragColor = texture(iInput, uv) * mix(1., min(k, 1.), iIntensity) * pow(defaultPulse, 2.);
}
