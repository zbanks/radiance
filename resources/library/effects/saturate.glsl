#property description Saturate colors in HSV space

void main(void) {
    vec4 origColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(demultiply(origColor).rgb);
    hsv.y = mix(hsv.y, 1., iIntensity * defaultPulse);
    fragColor.rgb = hsv2rgb(hsv);
    fragColor.a = 1.0;
    fragColor *= origColor.a;
}

