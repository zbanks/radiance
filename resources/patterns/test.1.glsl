void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    f_color0 = vec4(abs(uv - 0.5) / 0.5, 0., 1.);
}
