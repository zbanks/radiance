void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    if(iIndicator == 1) {
        f_color0 = composite(f_color0, vec4(1., 1., 0., 1.));
    } else {
        f_color0 = texture2D(iPreview, uv);
    }
}
