void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = uv.y * 0.5 + 0.1;
    float w = 4.;

    vec2 slider_origin = vec2(125., 50.);
    vec2 slider_gain = vec2(100., 0.);
    vec2 slider_pos = slider_origin + slider_gain * iIntensity;
    vec2 slider_size = vec2(10.);
    vec2 preview_origin = vec2(35., 100.);
    vec2 preview_size = vec2(280., 280.);

    if(iSelection) {
        gl_FragColor.a = 1.;
        gl_FragColor.rgb = dataColor(ivec3(3, 0, 0));
        gl_FragColor.rgb = mix(gl_FragColor.rgb, dataColor(ivec3(4, 0, 0)), inBox(gl_FragCoord.xy, slider_pos - slider_size, slider_pos + slider_size));
    } else {
        gl_FragColor = vec4(0.);
        float df = max(rounded_rect_df(vec2(175., 250.), vec2(130., 200.), 25.), 0.);
        gl_FragColor = composite(gl_FragColor, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));
        gl_FragColor = composite(gl_FragColor, vec4(0., 0.3, 0., smoothBox(gl_FragCoord.xy, slider_origin - vec2(w), slider_origin + slider_gain + vec2(w), w)));
        gl_FragColor = composite(gl_FragColor, vec4(0., 0.8, 0., smoothBox(gl_FragCoord.xy, slider_pos - slider_size, slider_pos + slider_size, w)));

        ivec2 grid_cell = ivec2(5. * (gl_FragCoord.xy - preview_origin) / preview_size);
        //vec3 grid = vec3(0.2) + vec3(0.1) * ((grid_cell.x + grid_cell.y) % 2);

        //vec3 grid = vec3(0., 0., 0.); // solid black
        //gl_FragColor = composite(gl_FragColor, vec4(grid, inBox(gl_FragCoord.xy, preview_origin, preview_origin + preview_size)));

        vec4 p = texture2D(iPreview, (gl_FragCoord.xy - preview_origin) / preview_size);
        vec4 p2 = texture2D(iStrips, (gl_FragCoord.xy - preview_origin) / preview_size);
        p.a *= inBox(gl_FragCoord.xy, preview_origin, preview_origin + preview_size);
        p2.a *= inBox(gl_FragCoord.xy, preview_origin, preview_origin + preview_size);

        if(iIndicator != 2) gl_FragColor = composite(gl_FragColor, p);
        if(iIndicator != 0) gl_FragColor = composite(gl_FragColor, p2);
    }

    //if(inBox(gl_FragCoord.xy, vec2(w), iResolution - vec2(w)) == 0.) {
    //    gl_FragColor = vec4(0.9, 0.9, 0.9, 1.);
    //} else if(length(gl_FragCoord.xy - slider) < 10) {
    //    gl_FragColor = vec4(0., 0., 0.5, 1.);
    //}
}
