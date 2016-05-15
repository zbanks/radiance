#version 130

uniform vec2 iResolution;

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = (1 - uv.y) * 0.2;
    gl_FragColor = vec4(g, g, g, 1.);
}
