#version 130

uniform vec2 iResolution;
uniform bool iSelection;

vec4 composite(vec4 under, vec4 over) {
    vec3 a_under = under.rgb * under.a;
    vec3 a_over = over.rgb * over.a;
    return vec4(a_over + a_under * (1. - over.a), over.a + under.a * (1 - over.a));
}

float rounded_rect_df(vec2 center, vec2 size, float radius) {
    return length(max(abs(gl_FragCoord.xy - center) - size, 0.0)) - radius;
}

vec4 fancy_rect(vec2 center) {
    float radius = 30.;
    vec2 size = vec2(75., 150.) - radius;
    float df = rounded_rect_df(center, size, radius);
    float shadow_df = rounded_rect_df(center + vec2(5., -5.), size, radius);
    vec4 color = vec4(vec3(0.1) * (center.y + size.y + radius - gl_FragCoord.y) / (2. * (size.y + radius)), clamp(1. - df, 0., 1.));
    vec4 shadow = vec4(0., 0., 0., clamp(0.5 - 0.05 * shadow_df, 0., 0.5));
    return composite(shadow, color);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = uv.y * 0.3;

    if(iSelection) {
        gl_FragColor = vec4(0., 0., 0., 1.);
    } else {
        gl_FragColor = vec4(g, g, g, 1.);
        gl_FragColor = composite(gl_FragColor, fancy_rect(vec2(175., 450.)));
    }
}
