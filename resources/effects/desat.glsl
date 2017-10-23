#property description Desaturate in YUV space

void main(void) {
    float factor = pow(iIntensity, 3.);

    vec4 samp = demultiply(texture(iInput, uv));

    //vec3 hsl = rgb2hsv(samp.rgb);
    //hsl.g *= 1.0 - factor;
    //fragColor.rgb = hsv2rgb(hsl);

    vec3 yuv = rgb2yuv(samp.rgb);
    yuv.gb -= 0.5;
    yuv.gb *= 1.0 - factor;
    yuv.gb += 0.5;
    fragColor.rgb = yuv2rgb(yuv);

    fragColor.a = samp.a;
    fragColor = premultiply(fragColor);
}
