void main(void) {
    vec2 uv = (gl_FragCoord.xy - iPosition) / iResolution;
    f_color0 = texture2D(iTexture, uv);
}
