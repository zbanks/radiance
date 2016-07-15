void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float rate = 2.;
    float r1 = sin(iIntensityIntegral * 2 * M_PI * rate) * iIntensity;
    float r2 = sawtooth(iIntensityIntegral, 0.0) * rate;
    float r = mix(r1, r2, iIntensity);
    float s = sin(r * 2. * M_PI);
    float c = cos(r * 2. * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * rot + 0.5);
}
