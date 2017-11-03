#property description The walls are melting
void main(void) {
    fragColor = texture(iInput, uv);
    vec2 perturb = sin(uv.yx * 10 + sin(vec2(iTime * 0.4, iTime * 0.6))); // Perturb a little to make the melting more wavy
    perturb *= 1. - smoothstep(0.9, 1., uv.y); // Don't perturb near the top to avoid going off-texture
    vec4 c = texture(iChannel[1], uv + 0.04 * iIntensity * perturb);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}
#buffershader

void main(void) {
    vec4 c1 = texture(iChannel[1], uv + vec2(0., 0.003));
    vec4 c2 = texture(iChannel[1], uv);
    fragColor = max(c2, c1); // Blend between the previous frame and a slightly shifted down version of it using the max function
    fragColor = max(fragColor - 0.005, vec4(0)); // Fade it out slightly

    // Get noise
    vec4 c = texture(iInput, uv);
    float s = texture(iNoise, uv / 150. + vec2(-iTime / 1000., 0.)).r;
    s *= sawtooth(iTime * 0.5, 0.9);

    // Threshold the noise based on intensity
    s = smoothstep(1. - iIntensity * 0.8, 1. - iIntensity * 0.7, s);

    // Show through the input video where the threshold is exceeded
    c *= s;
    fragColor = composite(c, fragColor);
    fragColor *= smoothstep(0, 0.2, iIntensity); // Clear back buffer when intensity is low
}
