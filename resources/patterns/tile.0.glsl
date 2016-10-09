// Repeating tiles

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    uv -= 0.5;
    float bins = pow(2, 4. * iIntensity);
    uv *= bins;
    uv += 0.5;

    // uv = mod(uv, vec2(1., 1.));

    uv = mod(uv, vec2(2., 2.));
    uv = vec2(1., 1.) - abs(vec2(1., 1.) - uv);

    gl_FragColor = texture2D(iFrame, uv);
}
