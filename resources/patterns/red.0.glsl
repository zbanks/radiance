// Change the color (in HSV) to red

void main(void) {
    vec4 c = texture2D(iFrame, v_uv);
    f_color0.r = mix(c.r, (c.r + c.g + c.b) / 3., iIntensity);
    f_color0.g = c.g * (1. - iIntensity);
    f_color0.b = c.b * (1. - iIntensity);
    f_color0.a = c.a;
}
