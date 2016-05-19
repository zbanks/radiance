#version 130

uniform vec2 iResolution;
uniform float iTime;

void main(void) {
    gl_FragColor = vec4(abs(gl_FragCoord.xy), 0., 1.);
}
