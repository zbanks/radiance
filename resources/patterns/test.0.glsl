#version 130

uniform vec2 iResolution;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;
uniform float iTime;

void main(void) {
    vec2 uv = gl_FragCoord.xy / 100.;
    uv.x += sin(iTime) * 0.2;
    uv.y += cos(iTime) * 0.2;
    //gl_FragColor = vec4(uv, 0.4, 1.0);
    gl_FragColor = texture2D(iChannel1, uv);
}
