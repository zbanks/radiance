// Reduce alpha

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    gl_FragColor *= (1. - iIntensity);
}
