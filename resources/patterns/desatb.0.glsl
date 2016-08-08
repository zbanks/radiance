// Desaturate to the beat

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float t = mod(iTime, 4.0) / 4.0;
    float factor = pow(iIntensity * t, 2.5);

    vec4 samp = texture2D(iFrame, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g *= 1.0 - factor;
    f_color0.rgb = hsv2rgb(hsl);
    f_color0.a = samp.a;
}
