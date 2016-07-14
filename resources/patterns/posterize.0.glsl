void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);

    float bins = min(256., 1. / iIntensity);

    gl_FragColor = round(gl_FragColor * bins) / bins;
}
