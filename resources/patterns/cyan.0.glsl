// Cyan diagonal stripes

void main(void) {
    f_color0 = texture2D(iFrame, v_uv);

    float t = v_uv.x * 3.0 + v_uv.y * 3.0;
    float y = smoothstep(0.2, 0.7, abs(mod(t - 3. * iIntensityIntegral, 2.) - 1.));
    float g = smoothstep(0.5, 0.9, abs(mod(1. + t - 3. * iIntensityIntegral, 2.) - 1.));

    vec4 c = vec4(0., 1., 1., y);
    c = composite(c, vec4(0., 0., 1., g * smoothstep(0.5, 0.8, iIntensity)));

    c.a *= smoothstep(0., 0.1, iIntensity);
    c = clamp(c, 0., 1.);
    f_color0 = composite(f_color0, c);
}
