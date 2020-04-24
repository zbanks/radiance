#property description Shake the image when there are lows

void main(void) {
    float t = iTime * 2. *  M_PI;
    vec2 sweep = vec2(cos(3. * t), sin(2. * t));

    float amount = iIntensity * 0.3 * max(iAudioLow - 0.2, 0.) * sawtooth(iTime, 0.2);

    vec2 newUV = (uv - 0.5) + sweep * amount + 0.5;

    fragColor = texture(iInput, newUV) * box(newUV);
}
