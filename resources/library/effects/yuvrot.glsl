#property description Shift the color in YUV space by rotating on the UV plane

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 yuv = rgb2yuv(demultiply(fragColor).rgb);

    float t = (iFrequency == 0.) ? (iIntensity * 2. * M_PI) : (iTime * iFrequency * M_PI);
    yuv.gb *= 2.;
    yuv.gb -= 1.;
    yuv.gb = vec2(yuv.g * cos(t) - yuv.b * sin(t),
                  yuv.g * sin(t) + yuv.b * cos(t));
    yuv.gb += 1.;
    yuv.gb /= 2.;

    fragColor.rgb = (iFrequency == 0.) ? yuv2rgb(yuv) * fragColor.a : mix(fragColor.rgb, yuv2rgb(yuv) * fragColor.a, iIntensity);
}
