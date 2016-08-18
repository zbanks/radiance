// Rotate the screen

void main(void) {
    float r = iIntensity;
    float s = sin(r * M_PI);
    float c = cos(r * M_PI);
    mat2 rot = mat2(c, -s, s, c);

    f_color0 = texture(iFrame, (v_uv - 0.5) * rot + 0.5);
}
