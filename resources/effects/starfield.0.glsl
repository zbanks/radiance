// Pixels radiating from the center

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec4 c = texture2D(iChannel[1], uv);
    c *= smoothstep(0., 0.2, iIntensity);
    gl_FragColor = composite(gl_FragColor, c);
}
