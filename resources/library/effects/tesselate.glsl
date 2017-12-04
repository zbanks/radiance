#property description Repeating tiles, flipped so that the pattern tesselates

void main(void) {
    vec2 normCoord = uv - 0.5;
    float bins = pow(2., 4. * iIntensity);
    vec2 newUV = normCoord * bins;
    newUV = abs(mod(newUV + 1.5, 2.) - 1.) - 0.5;
    fragColor = texture(iInput, newUV + 0.5);
}
