// Shift the color in YUV space by rotating on the UV plane

void main(void) {
    gl_FragColor = texture2D(iInput, uv);
    vec3 yuv = rgb2yuv(demultiply(gl_FragColor).rgb);

    float t = iIntensity * 2 * M_PI;
    yuv.gb *= 2.;
    yuv.gb -= 1.;
    yuv.g = yuv.g * cos(t) - yuv.b * sin(t);
    yuv.b = yuv.g * sin(t) + yuv.b * cos(t);
    yuv.gb += 1.;
    yuv.gb /= 2.;

    gl_FragColor.rgb = yuv2rgb(yuv) * gl_FragColor.a;
}
