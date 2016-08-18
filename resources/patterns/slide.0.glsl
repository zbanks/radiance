// Slide the screen left-to-right

void main(void) {
    float deviation = iIntensityIntegral;
    vec2 uv2 = v_uv;
    uv2.x = abs(mod(v_uv.x + deviation, 2.) - 1.);

    vec4 oc = texture(iFrame, v_uv);
    vec4 c = texture(iFrame, uv2);

    oc.a *= (1. - smoothstep(0.1, 0.2, iIntensity));
    c.a *= smoothstep(0, 0.1, iIntensity);

    f_color0 = composite(oc, c);
}
