// Per-pixel twinkle effect

void main(void) {
    f_color0 = texture(iChannel[0], v_uv);
    f_color0.a *= exp(-iIntensity / 20.);
    vec4 in_color = texture(iFrame, v_uv);

    if (rand(vec3(v_uv, iTime)) < exp(-iIntensity * 4.))
        f_color0 = in_color;
}
