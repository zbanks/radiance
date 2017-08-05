// Reduce alpha

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor.a *= (1. - iIntensity);
}
