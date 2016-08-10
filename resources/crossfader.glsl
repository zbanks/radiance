void main(void) {

    float left_alpha = min((1. - iIntensity) * 2., 1.);
    float right_alpha = min(iIntensity * 2., 1.);

    vec4 left = texture2D(iFrameLeft, v_uv);
    vec4 right = texture2D(iFrameRight, v_uv);

    left.a *= left_alpha;
    right.a *= right_alpha;

    if(iLeftOnTop) {
        f_color0 = composite(right, left);
    } else {
        f_color0 = composite(left, right);
    }
}
