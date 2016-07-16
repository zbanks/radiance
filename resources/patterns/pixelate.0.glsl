void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    uv -= 0.5;

    float bins = 256. * pow(2, -9. * iIntensity);

    uv = round(uv * bins) / bins;
    uv += 0.5;

    gl_FragColor = texture2D(iFrame, uv);
}
