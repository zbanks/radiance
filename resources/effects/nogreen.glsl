// Zero out the green channel (green is not a creative color)

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    gl_FragColor.g *= 1. - iIntensity;
}
