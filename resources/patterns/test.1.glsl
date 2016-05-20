#version 130

uniform vec2 iResolution;
uniform float iTime;

uniform mat3 iTransform;

void main(void) {
    vec3 uv = iTransform * vec3(gl_FragCoord.xy, 1.);
    gl_FragColor = vec4(abs(uv.x), abs(uv.y), 0., 1.);
}
