#property description Reduce number of colors in YUV space (but keep luminance)

void main(void) {
    //float bins = 256. * pow(2, -8. * iIntensity);
    float bins = min(256., 1. / iIntensity);
    
    // bin in non-premultiplied space, then re-premultiply
    fragColor = demultiply(texture(iInput, uv));
    vec4 c = fragColor;
    c.rgb = rgb2yuv(c.rgb);
    c.gb = clamp(round(c.gb * bins) / bins, 0.0, 1.0);
    c.rgb = yuv2rgb(c.rgb);
    fragColor = mix(fragColor, premultiply(c), pow(defaultPulse, 2.));
}
