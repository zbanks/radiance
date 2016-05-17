#version 130

uniform vec2 iResolution;
uniform bool iSelection;

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = (1 - uv.y) * 0.3;

    if(iSelection) {
        gl_FragColor = vec4(0., 0., 0., 1.);
    } else {
        gl_FragColor = vec4(g, g, g, 1.);
    }
}
