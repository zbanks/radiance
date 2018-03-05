#property description Per-pixel twinkle effect

void main(void) {
    fragColor = texture(iChannel[0], uv);
    fragColor *= exp(-iIntensity / 20.);
    if (rand(vec3(uv, iTime)) < exp(-iIntensity * 3.) * sawtooth(iTime * iFrequency, 0.9)) {
        fragColor = texture(iInput, uv);
    }
}
