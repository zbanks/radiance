// Zero out the everything but the green channel (green is not a creative color)

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    gl_FragColor.r *= 1. - iIntensity;
    gl_FragColor.b *= 1. - iIntensity;
}
