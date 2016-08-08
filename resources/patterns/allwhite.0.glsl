// Basic white fill

void main(void) {
    vec4 c = vec4(1., 1., 1., iIntensity);
    f_color0 = composite(texture2D(iFrame, v_uv), c);
}
