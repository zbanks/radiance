#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iLayer1;
uniform mat3 iTransform;

void main(void) {
    vec3 uv = iTransform * vec3(gl_FragCoord.xy, 1.);
    //gl_FragColor = texture2D(iLayer1, gl_FragCoord.xy / 2. + 0.5);
    gl_FragColor = vec4(abs(uv.x), 0., 0., 1.);
}
