void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    if(iIndicator == 1) {
        gl_FragColor = compositeCR(gl_FragColor, vec4(1., 1., 0., 1.));
    } else {
        gl_FragColor = texture2D(iPreview, uv);
    }
}
