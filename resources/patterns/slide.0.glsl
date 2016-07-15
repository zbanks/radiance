void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float deviation = iIntensityIntegral;
    vec2 uv2 = uv;
    uv2.x = abs(mod(uv.x + deviation, 2.) - 1.);

    vec4 oc = texture2D(iFrame, uv);
    vec4 c = texture2D(iFrame, uv2);

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    c.a *= smoothstep(0, 0.1, iIntensity);

    gl_FragColor = composite(oc, c);
}
