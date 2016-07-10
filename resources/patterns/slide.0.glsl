void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float deviation;
    deviation = smoothstep(0., 0.3, iIntensity) * mod(iIntensityIntegral, 1.);

    uv.x = mod(uv.x + deviation, 1.);

    gl_FragColor = texture2D(iFrame, uv);
}
