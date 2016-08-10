// Desaturate (make white)

void main(void) {

    float factor = pow(iIntensity, 3.);

    vec4 samp = texture2D(iFrame, v_uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g *= 1.0 - factor;
    f_color0.rgb = hsv2rgb(hsl);
    f_color0.a = samp.a;
}
