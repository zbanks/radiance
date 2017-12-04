#property description Pixelate/quantize the output

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float bs = 256. * pow(2., -9. * iIntensity);
    vec2 bins = bs * aspectCorrection;
    normCoord = round(normCoord * bins) / bins;

    vec2 newUV = normCoord / aspectCorrection + 0.5;

    fragColor = texture(iInput, newUV);
}
