#property description Shift the color in YUV space by rotating on the UV plane

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 yuv = rgb2yuv(demultiply(fragColor).rgb);

    float t = iIntensity * 2 * M_PI;
    yuv.gb *= 2.;
    yuv.gb -= 1.;
    yuv.gb = vec2(yuv.g * cos(t) - yuv.b * sin(t),
                  yuv.g * sin(t) + yuv.b * cos(t));
    yuv.gb += 1.;
    yuv.gb /= 2.;

    fragColor.rgb = yuv2rgb(yuv) * fragColor.a;
}
