// Desaturate (make white)

void main(void) {
    float factor = pow(iIntensity, 3.);

    vec4 samp = demultiply(texture2D(iInput, uv));

    //vec3 hsl = rgb2hsv(samp.rgb);
    //hsl.g *= 1.0 - factor;
    //gl_FragColor.rgb = hsv2rgb(hsl);

    vec3 yuv = rgb2yuv(samp.rgb);
    yuv.gb -= 0.5;
    yuv.gb *= 1.0 - factor;
    yuv.gb += 0.5;
    gl_FragColor.rgb = yuv2rgb(yuv);

    gl_FragColor.a = samp.a;
    gl_FragColor = premultiply(gl_FragColor);
}
