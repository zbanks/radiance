void main(void) {
    vec2 normCoord = 2. * (uv - 0.5) * aspectCorrection;
    gl_FragColor = vec4(abs(normCoord), 0., 1.);
}
