void main(void) {
    vec2 frag = v_uv*v_size;
    float g = v_uv.y * 0.5 + 0.1;
    float w = 4.;

    vec2 slider_origin = vec2(125., 50.);
    vec2 slider_gain = vec2(100., 0.);
    vec2 slider_pos = slider_origin + slider_gain * iIntensity;
    vec2 slider_size = vec2(10.);
    vec2 preview_origin = vec2(35., 100.);
    vec2 preview_size = vec2(280., 280.);

    f_color0 = vec4(0.);
    if(iSelection) {
        float df = max(rounded_rect_df(frag,vec2(175., 250.), vec2(130., 200.), RADIUS), 0.);
        if(df >= 1.0)
            discard;
        f_color0 = composite(f_color0, vec4(dataColor(ivec3(3, 0, 0)), rounded_rect_df(frag,vec2(176., 250.), vec2(130.,200.), RADIUS) <= 1.));
        f_color0.rgb = mix(f_color0.rgb, dataColor(ivec3(4, 0, 0)), inBox(frag.xy, slider_pos - slider_size, slider_pos + slider_size));
    } else {
        f_color0 = composite(f_color0, fancy_rect(frag,vec2(175, 250.), vec2(130., 200.), iSelected == 17 || iSelected == 18));
        float df = max(rounded_rect_df(frag,vec2(175., 250.), vec2(130., 200.), RADIUS), 0.);
        f_color0 = composite(f_color0, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));
        f_color0 = composite(f_color0, vec4(0., 0.3, 0., smoothBox(frag.xy, slider_origin - vec2(w), slider_origin + slider_gain + vec2(w), w)));
        f_color0 = composite(f_color0, vec4(0., 0.8, 0., smoothBox(frag.xy, slider_pos - slider_size, slider_pos + slider_size, w)));

//        ivec2 grid_cell = ivec2(5. * v_uv);
        //vec3 grid = vec3(0.2) + vec3(0.1) * ((grid_cell.x + grid_cell.y) % 2);

        //vec3 grid = vec3(0., 0., 0.); // solid black
        //f_color0 = composite(f_color0, vec4(grid, inBox(frag.xy, preview_origin, preview_origin + preview_size)));
        vec2 p_uv = (v_uv * v_size - preview_origin) / preview_size;
        vec4 p  = texture(iPreview, p_uv);
        vec4 p2 = texture(iStrips, p_uv);
        float in_pattern = float(inBox(frag.xy, preview_origin, preview_origin + preview_size));

        p.a  *= in_pattern;
        p2.a *= in_pattern;

        if(iIndicator != 2) f_color0 = composite(f_color0, p);
        if(iIndicator != 0) f_color0 = composite(f_color0, p2);
    }

}
