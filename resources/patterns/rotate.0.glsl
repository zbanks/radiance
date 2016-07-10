void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float s = sin(iIntensity * 2. * M_PI);
    float c = cos(iIntensity * 2. * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * rot + 0.5);
}
