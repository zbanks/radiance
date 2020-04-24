#property description That cool thing that happens when you beat solitaire

void main(void) {
    // Relative size of the bouncing image
    // I don't think we need to encorporate aspectCorrection?
    float scale = mix(1.0, 0.25, smoothstep(0., 0.3, iIntensity));

    // Closed-form bouncing behavior
    // I think this is periodic over [0.0, 16.] to prevent discontinuities
    vec2 xy = vec2(sawtooth(iStep * 0.3125, 0.5), 1.0 - abs(sin(iStep * M_PI * 0.5625)));

    xy = (xy - 0.5) * (1.0 - scale);
    vec2 uvSample = (uv - 0.5 + xy) / scale + 0.5;

    vec4 c = texture(iInput, uvSample) * box(uvSample);
    vec4 under = texture(iChannel[0], uv) * smoothstep(0., 0.1, iIntensity);
    fragColor = composite(under, c);
}
