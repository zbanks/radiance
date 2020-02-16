#property description That cool thing that happens when you beat solitaire
#property frequency 2

void main(void) {
    // Relative size of the bouncing image
    // I don't think we need to encorporate aspectCorrection?
    float scale = mix(1.0, 0.25, smoothstep(0., 0.3, iIntensity));

    // Closed-form bouncing behavior
    // I think this is periodic over [0.0, 16.] to prevent discontinuities
    float phase = iTime * iFrequency / 32.;
    vec2 xy = vec2(sawtooth(phase * 5.0, 0.5), 1.0 - abs(sin(phase * 9.0 * M_PI)));

    xy = (xy - 0.5) * (1.0 - scale);
    vec2 uvSample = (uv - 0.5 + xy) / scale + 0.5;

    vec4 c = texture(iInput, uvSample) * box(uvSample);
    vec4 under = texture(iChannel[0], uv) * smoothstep(0., 0.1, iIntensity);
    fragColor = composite(under, c);
}
