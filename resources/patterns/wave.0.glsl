// Green and blue base pattern

void main(void) {
    vec4 c = vec4(0., 0., 0., 1.);
    float ratio = 15;
    c.r = 0.0;
    c.g = sin((v_uv.x + v_uv.y) * ratio) / 2 + 0.5;
    c.b = sin((v_uv.x - v_uv.y) * ratio) / 2 + 0.5;
    c.a = iIntensity;

    f_color0 = composite(texture2D(iFrame, v_uv), c);
}
