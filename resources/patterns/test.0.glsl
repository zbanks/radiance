// A green & red circle in the center

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    f_color0 = texture2D(iFrame, uv);
    vec4 c;

    c = vec4(1., 1., 1., 1 - smoothstep(iIntensity - 0.1, iIntensity, length(uv - 0.5) / 0.5));
    f_color0 = composite(f_color0, c);

    c = texture2D(iChannel[1], (uv - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(uv - 0.5) / 0.5);
    f_color0 = composite(f_color0, c);
}
