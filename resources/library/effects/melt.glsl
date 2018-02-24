#property description The walls are melting
#property frequency 1
void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = texture(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    fragColor = composite(fragColor, c);
}

#buffershader

void main(void) {
    vec4 c1 = texture(iInput, uv);

    vec2 perturb = sin(uv.yx * 10. + sin(vec2(iTime * iFrequency * 0.5, iTime * iFrequency * 0.75))); // Perturb a little to make the melting more wavy
    perturb *= 1. - smoothstep(0.9, 1., uv.y); // Don't perturb near the top to avoid going off-texture

    vec4 c2 = texture(iChannel[1], uv + vec2(0., 0.01 * iIntensity) + 0.005 * iIntensity * perturb);

    fragColor = max(c1, c2); // Blend between the current frame and a slightly shifted down version of it using the max function
    fragColor = max(fragColor - 0.002 - 0.02 * (1. - iIntensity), vec4(0)); // Fade it out slightly

    fragColor *= smoothstep(0., 0.1, iIntensity); // Clear back buffer when intensity is low
}
