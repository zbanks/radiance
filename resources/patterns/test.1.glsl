#version 130

uniform vec2 iResolution;
uniform float iTime;

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = vec4(abs(uv - 0.5) / 0.5, 0., 1.);
}
