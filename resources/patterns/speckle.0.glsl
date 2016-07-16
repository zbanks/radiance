void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    gl_FragColor = texture2D(iChannel[0], uv);
    gl_FragColor.a *= exp(-iIntensity / 20.);
    if (rand(vec3(uv, iTime)) < exp(-iIntensity * 4.))
        gl_FragColor = texture2D(iFrame, uv);
}
