// Fake edge detection based only on alpha

void main(void) {
    f_color0 = texture(iFrame, v_uv);
    f_color0.a = mix(f_color0.a, f_color0.a * (iIntensity - f_color0.a) / 0.25, iIntensity);
}
