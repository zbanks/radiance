// Fake edge detection based only on alpha

void main(void) {
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor.a = mix(gl_FragColor.a, gl_FragColor.a * (iIntensity - gl_FragColor.a) / 0.25, iIntensity);
}
