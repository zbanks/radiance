// Desaturate (make white)

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float factor = pow(iIntensity, 3.);

    vec4 samp = texture2D(iFrame, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g *= 1.0 - factor;
    f_color0.rgb = hsv2rgb(hsl);
    f_color0.a = samp.a;
}
