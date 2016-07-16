void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float left_alpha = min((1. - iIntensity) * 2., 1.);
    float right_alpha = min(iIntensity * 2., 1.);

    vec4 left = texture2D(iFrameLeft, uv);
    vec4 right = texture2D(iFrameRight, uv);

    left.a *= left_alpha;
    right.a *= right_alpha;

    if(iLeftOnTop) {
        gl_FragColor = composite(right, left);
    } else {
        gl_FragColor = composite(left, right);
    }
}
