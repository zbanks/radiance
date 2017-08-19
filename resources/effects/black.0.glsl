// Reduce alpha

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor *= (1. - iIntensity);
}
