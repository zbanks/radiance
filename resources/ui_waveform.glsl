void main(void) {
//    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = v_uv.y * 0.5 + 0.1;
    float w = 4.;

    f_color0 = vec4(0.);

    float df = max(rounded_rect_df(vec2(200., 100.), vec2(165., 65.), 25.), 0.);

    float shrink_freq = 190. / 200.;
    float shrink_mag = 90. / 100.;
    float freq = (v_uv.x - 0.5) / shrink_freq + 0.5;
    float mag = abs(v_uv.y - 0.5) * shrink_mag;
    vec4 wav = texture1D(iWaveform, freq);
    vec4 beats = texture1D(iBeats, freq);
    vec3 wf = (wav.rgb - mag) * 90.;
    wf = smoothstep(0., 1., wf) * (1. - step(1., df));
    float level = (wav.a - mag) * 90;
    level = (smoothstep(-5., 0., level) - smoothstep(0., 5., level)) * (1 - step(1., df));
    float beat = beats.x;
    beat = beat * (1. - step(1., df));
    //float rg = 0.5 * clamp(0., 1., d.r / 30.);
    f_color0 = composite(f_color0, vec4(0.5, 0.1, 0.1, beat));
    f_color0 = composite(f_color0, vec4(0.0, 0.0, 0.6, wf.b));
    f_color0 = composite(f_color0, vec4(0.3, 0.3, 1.0, wf.g));
    f_color0 = composite(f_color0, vec4(0.7, 0.7, 1.0, wf.r));
    f_color0 = composite(f_color0, vec4(0.7, 0.7, 0.7, level * 0.5));
    f_color0 = composite(f_color0, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));
}
