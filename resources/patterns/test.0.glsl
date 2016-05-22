#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iLayer1;
uniform sampler2D iFrame;
uniform mat3 iTransform;
uniform float iIntensity;

vec4 composite(vec4 under, vec4 over) {
    vec3 a_under = under.rgb * under.a;
    vec3 a_over = over.rgb * over.a;
    return vec4(a_over + a_under * (1. - over.a), over.a + under.a * (1 - over.a));
}

void main(void) {
    vec3 uv = iTransform * vec3(gl_FragCoord.xy, 1.);
    gl_FragColor = texture2D(iFrame, gl_FragCoord.xy / iResolution);

    vec4 c;

    c = vec4(1., 1., 1., 1 - smoothstep(iIntensity - 0.1, iIntensity, length(uv.xy)));
    gl_FragColor = composite(gl_FragColor, c);

    c = texture2D(iLayer1, (gl_FragCoord.xy / iResolution - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(uv.xy));
    gl_FragColor = composite(gl_FragColor, c);

}
