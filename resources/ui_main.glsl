#version 130

uniform vec2 iResolution;
uniform bool iSelection;
uniform int iSelected;

float RADIUS = 25.;

vec4 composite(vec4 under, vec4 over) {
    return vec4(over.rgb * over.a  + under.rgb * under.a * (1. - over.a), over.a + under.a * (1. - over.a));
}

float rounded_rect_df(vec2 center, vec2 size, float radius) {
    return length(max(abs(gl_FragCoord.xy - center) - size, 0.0)) - radius;
}

vec4 fancy_rect(vec2 center, vec2 size, bool selected) {
    vec4 c;
    vec4 color;

    if(selected) {
        float highlight_df = rounded_rect_df(center, size, RADIUS - 10.);
        color = vec4(1., 1., 0., 0.5 * (1. - smoothstep(0., 50., max(highlight_df, 0.))));
    } else {
        float shadow_df = rounded_rect_df(center + vec2(10., -10.), size, RADIUS - 10.);
        color = vec4(0., 0., 0., 0.5 * (1. - smoothstep(0., 20., max(shadow_df, 0.))));
    }

    float df = rounded_rect_df(center, size, RADIUS);
    c = vec4(vec3(0.1) * (center.y + size.y + RADIUS - gl_FragCoord.y) / (2. * (size.y + RADIUS)), clamp(1. - df, 0., 1.));
    color = composite(color, c);
    return color;
}

vec3 dataColor(ivec3 data) {
    return vec3(data) / vec3(255.);
}

vec2 PAT_SIZE = vec2(45., 75.);

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    if(iSelection) {
        gl_FragColor = vec4(0., 0., 0., 1.);
        for(int i=0; i < 9; i++) {
            vec2 p = vec2(175. + i * 200., 200.);
            ivec3 c;
            if(i < 4) {
                c = ivec3(1, i, 0);
            } else if(i == 4) {
                c = ivec3(3, 0, 0);
            } else {
                c = ivec3(1, i - 1, 0);
            }
            gl_FragColor = composite(gl_FragColor, vec4(dataColor(c), rounded_rect_df(p, PAT_SIZE, RADIUS) <= 1.));
        }
    } else {
        float g = uv.y * 0.1 + 0.2;
        gl_FragColor = vec4(g, g, g, 1.);
        for(int i=0; i < 9; i++) {
            vec2 p = vec2(175. + i * 200., 225.);
            gl_FragColor = composite(gl_FragColor, fancy_rect(p, PAT_SIZE, iSelected == i + 1));
        }
        gl_FragColor = composite(gl_FragColor, fancy_rect(vec2(300., 650.), vec2(165., 65.), false));
        gl_FragColor = composite(gl_FragColor, fancy_rect(vec2(300., 450.), vec2(165., 65.), false));
    }
}
