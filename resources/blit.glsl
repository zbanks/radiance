void main(void) {
    vec2 uv = (gl_FragCoord.xy - iPosition) / iResolution;
    gl_FragColor = texture2D(iTexture, uv);
}
