#version 130

uniform vec2 iResolution;
uniform sampler2D iFrameLeft;
uniform sampler2D iFrameRight;
uniform float iPosition;
uniform bool iLeftOnTop;

vec4 composite(vec4 under, vec4 over) {
    vec3 a_under = under.rgb * under.a;
    vec3 a_over = over.rgb * over.a;
    return vec4(a_over + a_under * (1. - over.a), over.a + under.a * (1 - over.a));
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrameLeft, uv);
    gl_FragColor = composite(gl_FragColor, texture2D(iFrameRight, uv));
}
