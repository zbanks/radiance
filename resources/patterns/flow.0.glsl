// Radiate color from the center based on audio

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    f_color0 = texture2D(iFrame, uv);
    vec4 c = texture2D(iChannel[1], uv);
    c.a *= smoothstep(0., 0.2, iIntensity);
    f_color0 = composite(c, f_color0);
}
