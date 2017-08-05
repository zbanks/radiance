// Basic white fill

void main(void) {
    vec4 c = vec4(1., 1., 1., iIntensity);
    gl_FragColor = composite(texture2D(iFrame, uv), c);
}
