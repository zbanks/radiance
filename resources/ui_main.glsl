#version 130

uniform vec2 iResolution;
uniform bool iSelection;
uniform int iSelected;

float RADIUS = 25.;
vec2 SIZE = vec2(45., 120.);

vec4 composite(vec4 under, vec4 over) {
    return vec4(over.rgb * over.a  + under.rgb * under.a * (1. - over.a), over.a + under.a * (1. - over.a));
}

float rounded_rect_df(vec2 center, vec2 size, float radius) {
    return length(max(abs(gl_FragCoord.xy - center) - size, 0.0)) - radius;
}

vec4 fancy_rect(vec2 center, bool selected) {
    vec4 c;
    float shadow_df = rounded_rect_df(center + vec2(5., -5.), SIZE, RADIUS - 10.);
    vec4 color = vec4(0., 0., 0., 0.5 * (1. - smoothstep(max(shadow_df, 0.), 0., 10.)));

    if(selected) {
        float highlight_df = rounded_rect_df(center, SIZE, RADIUS - 10.);
        c = vec4(1., 1., 0., 0.5 * (1. - smoothstep(max(highlight_df, 0.), 0., 20.)));
        color = composite(color, c);
    }

    float df = rounded_rect_df(center, SIZE, RADIUS);
    c = vec4(vec3(0.1) * (center.y + SIZE.y + RADIUS - gl_FragCoord.y) / (2. * (SIZE.y + RADIUS)), clamp(1. - df, 0., 1.));
    color = composite(color, c);
    return color;
}

vec3 dataColor(ivec3 data) {
    return vec3(data) / vec3(255.);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = uv.y * 0.1 + 0.2;

    if(iSelection) {
        gl_FragColor = vec4(0., 0., 0., 1.);
        for(int i=0; i < 9; i++) {
            vec2 p = vec2(175. + i * 200., 450.);
            ivec3 c;
            if(i < 4) {
                c = ivec3(1, i, 0);
            } else if(i == 4) {
                c = ivec3(3, 0, 0);
            } else {
                c = ivec3(1, i - 1, 0);
            }
            gl_FragColor = composite(gl_FragColor, vec4(dataColor(c), rounded_rect_df(p, SIZE, RADIUS) <= 1.));
        }
    } else {
        gl_FragColor = vec4(g, g, g, 1.);
        for(int i=0; i < 9; i++) {
            vec2 p = vec2(175. + i * 200., 450.);
            gl_FragColor = composite(gl_FragColor, fancy_rect(p, iSelected == i + 1));
        }
    }
}
