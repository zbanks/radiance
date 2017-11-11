#property description Turns the image into tiles and flips each one along a diagonal

void main(void) {
    vec2 timeOffset = vec2(iIntensityIntegral * 0.06) * sign(uv.x + uv.y - 1.);
    vec2 newPt = (uv - 0.5 - timeOffset) * aspectCorrection;

    float bins = mix(50., 3., iIntensity);

    vec2 newPtInt = floor(newPt * bins);
    vec2 newPtFrac = fract(newPt * bins);

    newPtFrac = 1. - newPtFrac.yx;

    newPt = (newPtInt + newPtFrac) / bins;

    fragColor = texture(iInput, newPt / aspectCorrection + 0.5 + timeOffset);
    fragColor = mix(texture(iInput, uv), fragColor, smoothstep(0., 0.1, iIntensity));
}
