#property description Repeating tiles

void main(void) {
    vec2 normCoord = uv - 0.5;
    float bins = pow(2., 4. * iIntensity);
    vec2 newUV = normCoord * bins;
    newUV = mod(newUV + 0.5, 1.) - 0.5;
    fragColor = texture(iInput, newUV + 0.5);
}
