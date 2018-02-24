#property description De-sync the shutter and the film feed

void main(void) {

    // Lookup what offset to display the picture at
    float offset = texture(iChannel[1], vec2(0.5, 0.5)).r;

    // Display the frame from iChannel[2] at that offset
    vec4 c = texture(iChannel[2], mod(uv + vec2(0., offset), 1.));

    fragColor = c;
}

#buffershader

// This shader integrates iIntensity
// on a per-frame basis
// to maintain vsync
// (all pixels are the same color)

void main(void) {
    float parameter = iIntensity * sawtooth(iTime * iFrequency * 0.25, 0.5);
    float v = mod(texture(iChannel[1], vec2(0.5, 0.5)).r - parameter, 1.);

    // If intensity is low, decay to zero
    v = max(0., v - 0.02 * (1. - step(0.03, iIntensity)));

    fragColor = vec4(v);
}

#buffershader

// This shader mirrors the input when intensity is low,
// and freezes the frame more and more as the intensity increases

void main(void) {
    float freeze = step(0.8, iIntensity);
    fragColor = mix(texture(iInputs[0], uv), texture(iChannel[2], uv), freeze);
}
