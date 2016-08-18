// A green & red circle in the center

void main(void) {
    f_color0 = texture(iFrame, v_uv);
    vec4 c;

    c = vec4(1., 1., 1., 1 - smoothstep(iIntensity - 0.1, iIntensity, length(v_uv - 0.5) / 0.5));
    f_color0 = composite(f_color0, c);

    c = texture(iChannel[1], (v_uv - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(v_uv - 0.5) / 0.5);
    f_color0 = composite(f_color0, c);
}
