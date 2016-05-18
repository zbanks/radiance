#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iLayer1;

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    uv.x += sin(iTime) * 0.2;
    uv.y += cos(iTime) * 0.2;
    //gl_FragColor = vec4(uv, 0.4, 1.0);
    gl_FragColor = texture2D(iLayer1, uv);
}
