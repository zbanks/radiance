#version 130

uniform vec2 iResolution;
uniform sampler2D iFrameLeft;
uniform sampler2D iFrameRight;
uniform float iPosition;
uniform bool iLeftOnTop;

vec4 composite(vec4 under, vec4 over) {
    float a_out = over.a + under.a * (1. - over.a);
    return vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float left_alpha = min((1. - iPosition) * 2., 1.);
    float right_alpha = min(iPosition * 2., 1.);

    vec4 left = texture2D(iFrameLeft, uv);
    vec4 right = texture2D(iFrameRight, uv);

    left.a *= left_alpha;
    right.a *= right_alpha;

    if(iLeftOnTop) {
        gl_FragColor = composite(right, left);
    } else {
        gl_FragColor = composite(left, right);
    }
}
