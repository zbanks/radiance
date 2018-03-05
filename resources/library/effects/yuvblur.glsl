#property description Horizontal blur of UV channels to give an old-timey effect
// This effect isn't actually that good...

#define MAX_DELTA 0.04

void main(void) {
    fragColor = texture(iInput, uv);
    vec3 yuv = rgb2yuv(demultiply(fragColor).rgb);

    float delta = MAX_DELTA * iIntensity * pow(defaultPulse, 2.);
    vec3 left  = rgb2yuv(demultiply(texture(iInput, uv + vec2(-delta, 0))).rgb);
    vec3 right = rgb2yuv(demultiply(texture(iInput, uv + vec2(+delta, 0))).rgb);
    yuv.gb = mix(yuv.gb, (left.gb + right.gb) / 2.0, 0.7 * iIntensity);
    yuv.gb = clamp(yuv.gb, 0., 1.);

    fragColor.rgb = yuv2rgb(yuv) * fragColor.a;
}
