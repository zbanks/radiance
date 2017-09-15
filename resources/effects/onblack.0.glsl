// Composite the input image onto black

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    gl_FragColor.rgb *= mix(1.0, gl_FragColor.a, iIntensity);
    gl_FragColor.a = mix(gl_FragColor.a, 1.0, iIntensity);
}
