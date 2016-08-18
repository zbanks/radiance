float RADIUS = 10.;
float SHRINK = 30.;

void main(void) {
    vec2 size = v_size / 2 - RADIUS - SHRINK;
    vec2 center = v_size / 2;
    vec2 fragcoord = v_uv * v_size;
    float shadow_df = rounded_rect_df(fragcoord,center + vec2(10., -10.), size, RADIUS - 5.);
    vec4 color = vec4(0., 0., 0., 0.5 * (1. - smoothstep(0., 20., max(shadow_df, 0.))));

    vec4 c;
    float df = max(rounded_rect_df(center, size, RADIUS), 0.);
    color = composite(color, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));
    c = vec4(vec3(0.1) * (center.y + size.y + RADIUS - fragcoord.y) / (2. * (size.y + RADIUS)), clamp(1. - df, 0., 1.));
    color = composite(color, c);

    vec2 tex_coord = (vec2(1, -1) * (fragcoord.xy - vec2(RADIUS + SHRINK, 45. + iTextResolution.y))) / iTextResolution;
    c = texture2D(iText, tex_coord);
    c.a *= 1. - smoothstep(0., 1., df);
    vec2 in_box = step(vec2(0.), tex_coord) - step(vec2(1.), tex_coord);
    c.a *= in_box.x * in_box.y;
    color = composite(color, c);

    f_color0 = color;
}
