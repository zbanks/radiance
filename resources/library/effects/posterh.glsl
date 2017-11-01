#property description Reduce number of hues in HSV space

void main(void) {
    float bins = min(360., 6. / iIntensity);
    
    vec4 hsv = demultiply(texture(iInput, uv));
    hsv.rgb = rgb2hsv(hsv.rgb);
    hsv.r = mod(round(hsv.r * bins) / bins, 1.0);
    hsv.rgb = hsv2rgb(hsv.rgb);

    fragColor = premultiply(hsv);
}
