void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec2 deviation = vec2(iIntensityIntegral, 0.);

    vec4 oc = texture2D(iFrame, uv);
    vec4 c = texture2D(iFrame, abs(mod(uv + deviation, 2.) - 1.));

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    c.a *= smoothstep(0, 0.1, iIntensity);

    gl_FragColor = composite(oc, c);
}
