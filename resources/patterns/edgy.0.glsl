void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    gl_FragColor.a = mix(gl_FragColor.a, gl_FragColor.a * (iIntensity - gl_FragColor.a) / 0.25, iIntensity);
}
