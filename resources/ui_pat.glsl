void main(void) {
    float g = v_uv.y * 0.5 + 0.1;
    float w = 4.;

    vec2 slider_origin = vec2(25., 45.);
    vec2 slider_gain = vec2(100., 0.);
    vec2 slider_pos = slider_origin + slider_gain * iIntensity;
    vec2 slider_size = vec2(10.);
    vec2 preview_origin = vec2(25., 75.);
    vec2 preview_size = vec2(100., 100.);
    vec2 name_origin = vec2(25., 210.);

    if(iSelection) {
        f_color0.a = 1.;
        f_color0.rgb = dataColor(ivec3(1, iPatternIndex, 0));
        f_color0.rgb = mix(f_color0.rgb, dataColor(ivec3(2, iPatternIndex, 0)), inBox(gl_FragCoord.xy, slider_pos - slider_size, slider_pos + slider_size));
    } else {
        f_color0 = vec4(0.);
        /*
        f_color0 = composite(f_color0, vec4(0.2, 0.2, 0.2, 0.8));
        f_color0 = composite(f_color0, vec4(1., 1., 1., (1. - smoothBox(gl_FragCoord.xy, vec2(w), iResolution - vec2(w), w))));
        */

        float df = max(rounded_rect_df(vec2(75., 125.), vec2(45., 75.), 25.), 0.);
        f_color0 = composite(f_color0, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));

        f_color0 = composite(f_color0, vec4(0., 0., 0.3, smoothBox(v_uv * v_size, slider_origin - vec2(w), slider_origin + slider_gain + vec2(w), w)));
        f_color0 = composite(f_color0, vec4(0., 0., 0.8, smoothBox(v_uv * v_size, slider_pos - slider_size, slider_pos + slider_size, w)));

        ivec2 grid_cell = ivec2(5. * (gl_FragCoord.xy - preview_origin) / preview_size);
        vec3 grid = vec3(0.2) + vec3(0.1) * ((grid_cell.x + grid_cell.y) % 2);
        f_color0 = composite(f_color0, vec4(grid, inBox(gl_FragCoord.xy, preview_origin, preview_origin + preview_size)));

        vec4 p = texture2D(iPreview, (gl_FragCoord.xy - preview_origin) / preview_size);
        p.a *= inBox(v_uv * v_size, preview_origin, preview_origin + preview_size);
        f_color0 = composite(f_color0, p);

        vec2 tex_coord = vec2(1., -1.) * v_uv;
        vec4 n = texture2D(iName, tex_coord);
        vec2 in_box = step(vec2(0.), tex_coord) - step(vec2(1.), tex_coord);
        n.a *= in_box.x * in_box.y;
        f_color0 = composite(f_color0, n);
    }

    //if(inBox(gl_FragCoord.xy, vec2(w), iResolution - vec2(w)) == 0.) {
    //    f_color0 = vec4(0.9, 0.9, 0.9, 1.);
    //} else if(length(gl_FragCoord.xy - slider) < 10) {
    //    f_color0 = vec4(0., 0., 0.5, 1.);
    //}
}
