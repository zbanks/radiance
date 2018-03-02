#property description Saturate colors in YUV space by making things more UV

void main(void) {
    fragColor = texture(iInput, uv);
    vec4 c = demultiply(fragColor);
    c.rgb = rgb2yuv(c.rgb);

    vec2 d = c.gb - vec2(0.5, 0.5);
    c.gb += d * iIntensity * 3.0;

    c.gb = clamp(c.gb, 0., 1.);
    c.rgb = yuv2rgb(c.rgb);
    fragColor = mix(fragColor, premultiply(c), pow(defaultPulse, 2.));
}
