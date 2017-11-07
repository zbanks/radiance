#property description Pass-thru when there are "highs" in the music

void main(void) {
    float k = iAudioHi * 2.;
    fragColor = texture(iInput, uv) * mix(1., min(k, 1.), iIntensity);
}
