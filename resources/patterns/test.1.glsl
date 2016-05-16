#version 130

uniform vec2 iResolution;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;
uniform float iTime;

void main(void) {
    vec2 uv = gl_FragCoord.xy / 100.;
    gl_FragColor = vec4(uv, sin(iTime), 1.);
}
