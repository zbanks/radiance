// Black sine wave from left to right.

void main(void) {
    vec2 normCoord = (uv - 0.5) * aspectCorrection;
    float x = (normCoord.x + normCoord.y) * 15. + iTime;
    gl_FragColor = texture2D(iInput, uv);
    gl_FragColor *= 1.0 - iIntensity * (0.5 * sin(x) + 0.5);
}
