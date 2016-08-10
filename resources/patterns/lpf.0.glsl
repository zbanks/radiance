// Smooth output

void main(void) {

    vec4 prev = texture2D(iChannel[0], v_uv);
    vec4 next = texture2D(iFrame, v_uv);
    prev.a *= 0.98;
    f_color0 = mix(next, prev, pow(iIntensity, 0.4));
}
