#property description Saturate colors in HSV space

void main(void) {
    vec4 origColor = texture(iInput, uv);
    vec3 hsv = rgb2hsv(demultiply(origColor).rgb);
    //hsv.yz = mix(hsv.yz, vec2(1.0, 1.0), iIntensity);
    hsv.y = mix(hsv.y, 1., iIntensity);
    fragColor.rgb = hsv2rgb(hsv);
    fragColor.a = 1.0;
    fragColor *= origColor.a;
}

