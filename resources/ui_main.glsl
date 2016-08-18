void main(void) {
    f_color0 = vec4(0.);
    vec2 frag = v_uv * v_size;
    if(iSelection) {
        f_color0 = vec4(0., 0., 0., 1.);
    } else {
        float g = v_uv.y * 0.1 + 0.2;
        f_color0 = vec4(g, g, g, 1.);

        glow(vec2(475., iLeftDeckSelector == 0 ? 420. : 180.),f_color0);
        glow(vec2(1475., iRightDeckSelector == 1 ? 420. : 180.),f_color0);
    }
}
