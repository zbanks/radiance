void main(void) {

    f_color0 = texture2D(iChannel[1], (v_uv - 0.5) * 0.98 + 0.5);
    f_color0.a *= exp((iIntensity - 2.) / 50.) * smoothstep(0, 0.01, length(v_uv - 0.5));

    vec4 c = texture2D(iFrame, v_uv);
    float s = smoothstep(0.90, 1., 1. - mod(iTime, 1.)) * iAudioLevel;
    c.a *=  min(3. * s, 1.);
    f_color0 = composite(f_color0, c);
}
