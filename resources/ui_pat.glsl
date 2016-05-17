#version 130

uniform vec2 iResolution;
uniform int iPatternIndex;
uniform float iIntensity;
uniform bool iSelection;

float inBox(vec2 coord, vec2 bottomLeft, vec2 topRight) {
    vec2 a = step(bottomLeft, coord) - step(topRight, coord);
    return a.x * a.y;
}

float smoothBox(vec2 coord, vec2 bottomLeft, vec2 topRight, float width) {
    vec2 a = smoothstep(bottomLeft, bottomLeft + vec2(width), coord) - smoothstep(topRight - vec2(width), topRight, coord);
    return min(a.x, a.y);
}


vec4 dataColor(ivec3 data) {
    return vec4(float(data.x) / 255., float(data.y) / 255., float(data.z) / 255., 1.);
}

vec4 composite(vec4 under, vec4 over) {
    vec3 a_under = under.rgb * under.a;
    vec3 a_over = over.rgb * over.a;
    return vec4(a_over + a_under * (1. - over.a), over.a + under.a * (1 - over.a));
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = uv.y * 0.5 + 0.1;
    float w = 2.;

    vec2 slider_origin = vec2(iResolution.x / 2. - 50., 100.);
    vec2 slider_gain = vec2(100., 0);
    vec2 slider_pos = slider_origin + slider_gain * iIntensity;

    if(iSelection) {
        gl_FragColor = dataColor(ivec3(1, iPatternIndex, 0));
    } else {
        gl_FragColor = vec4(0.);
        gl_FragColor = composite(gl_FragColor, vec4(0.2, 0.2, 0.2, 0.8));
        gl_FragColor = composite(gl_FragColor, vec4(1., 1., 1., (1. - smoothBox(gl_FragCoord.xy, vec2(w), iResolution - vec2(w), w))));
        gl_FragColor = composite(gl_FragColor, vec4(0., 0., 0.3, smoothBox(gl_FragCoord.xy, slider_origin - vec2(w), slider_origin + slider_gain + vec2(w), w)));
        gl_FragColor = composite(gl_FragColor, vec4(0., 0., 1., smoothBox(gl_FragCoord.xy, slider_pos - vec2(10), slider_pos + vec2(10), w)));
    }



    //if(inBox(gl_FragCoord.xy, vec2(w), iResolution - vec2(w)) == 0.) {
    //    gl_FragColor = vec4(0.9, 0.9, 0.9, 1.);
    //} else if(length(gl_FragCoord.xy - slider) < 10) {
    //    gl_FragColor = vec4(0., 0., 0.5, 1.);
    //}
}
