#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iLayer1;
uniform mat3 iTransform;
uniform float iIntensity;

void main(void) {
    vec3 uv = iTransform * vec3(gl_FragCoord.xy, 1.);
    gl_FragColor = texture2D(iLayer1, gl_FragCoord.xy / iResolution);
    gl_FragColor.a = 1. - smoothstep(iIntensity - 0.1, iIntensity, length(uv.xy));
}
