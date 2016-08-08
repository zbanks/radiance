// Green and blue base pattern

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec4 c = vec4(0., 0., 0., 1.);
    float ratio = 15;
    c.r = 0.0;
    c.g = sin((uv.x + uv.y) * ratio) / 2 + 0.5;
    c.b = sin((uv.x - uv.y) * ratio) / 2 + 0.5;
    c.a = iIntensity;

    f_color0 = composite(texture2D(iFrame, uv), c);
}
