#property description Desaturate to the beat

void main(void) {
    float t = mod(iTime, 4.0) / 4.0;
    float factor = pow(iIntensity * t, 2.5);

    vec4 samp = texture(iInput, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g *= 1.0 - factor;
    fragColor.rgb = hsv2rgb(hsl);
    fragColor.a = samp.a;
    fragColor = premultiply(fragColor);
}
