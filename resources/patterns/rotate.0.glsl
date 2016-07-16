void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float r = iIntensity;
    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * rot + 0.5);
}
