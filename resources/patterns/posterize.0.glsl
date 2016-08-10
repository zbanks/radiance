// Reduce number of colors

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);

    //float bins = 256. * pow(2, -8. * iIntensity);
    float bins = min(256., 1. / iIntensity);

    f_color0 = round(f_color0 * bins) / bins;
}
