#property description Pixelate/quantize the output horizontally

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float bs = 256. * pow(2., -9. * iIntensity);
    bs *= 0.7 + 0.3 * pow(defaultPulse, 2.);
    float bins = bs * aspectCorrection.y;
    normCoord.y = round(normCoord.y * bins) / bins;

    vec2 newUV = normCoord / aspectCorrection + 0.5;

    fragColor = texture(iInput, newUV);
}
