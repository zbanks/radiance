// Change the color (in HSV) to red

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec4 c = texture2D(iFrame, uv);
    f_color0.r = mix(c.r, (c.r + c.g + c.b) / 3., iIntensity);
    f_color0.g = c.g * (1. - iIntensity);
    f_color0.b = c.b * (1. - iIntensity);
    f_color0.a = c.a;
}
