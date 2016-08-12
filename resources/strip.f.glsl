
void main(void) {
    f_color0 = vec4(1.);
    if(iIndicator == 1) {
        f_color0 = composite(f_color0, vec4(1., 1., 0., 1.));
    } else {
        f_color0 = texture2D(iPreview, v_uv);
    }
}
