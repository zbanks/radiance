// Pixelate/quantize the output

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;

    float bins = 256. * pow(2, -9. * iIntensity);

    normCoord = round(normCoord * bins) / bins;

    vec2 newUV = normCoord / aspectCorrection + 0.5;

    gl_FragColor = texture2D(iFrame, newUV);
}
