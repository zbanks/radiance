void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    float r = 0.2;

    c = vec4(1., 0.5, 0.5, 1 - smoothstep(r - 0.1, r, length(mod((uv - 0.5) * 5. * iIntensity - 0.5, 1.) - 0.5)));
    c.a *= smoothstep(0., 0.2, iIntensity);
    gl_FragColor = composite(gl_FragColor, c);
}
