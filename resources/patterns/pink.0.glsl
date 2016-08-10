// Pink polka dots

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);
    vec4 c;

    float r = 0.2;

    c = vec4(1., 0.5, 0.5, 1 - smoothstep(r - 0.1, r, length(mod((v_uv - 0.5) * 5. * iIntensity - 0.5, 1.) - 0.5)));
    c.a *= smoothstep(0., 0.2, iIntensity);
    f_color0 = composite(f_color0, c);
}
