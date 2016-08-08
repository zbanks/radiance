// Reduce alpha

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);
    f_color0.a *= (1. - iIntensity);
}
