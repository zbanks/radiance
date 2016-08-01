// Desaturate (make white)

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float factor = pow(iIntensity, 3.);

    vec4 samp = texture2D(iFrame, uv);
    vec3 hsl = rgb2hsv(samp.rgb);
    hsl.g *= 1.0 - factor;
    gl_FragColor.rgb = hsv2rgb(hsl);
    gl_FragColor.a = samp.a;
}
