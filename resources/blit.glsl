#version 130

uniform vec2 iPosition;
uniform vec2 iResolution;
uniform sampler2D iTexture;

void main(void) {
    vec2 uv = (gl_FragCoord.xy - iPosition) / iResolution;
    gl_FragColor = texture2D(iTexture, uv);
}
