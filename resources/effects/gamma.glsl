#property description Brighten the image using gamma correction

void main(void) {
    fragColor = texture(iInput, uv);
    fragColor = pow(fragColor, vec4(1. / (1. + iIntensity * 3.)));
}

