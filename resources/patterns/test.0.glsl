#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iChannel[2];
uniform sampler2D iFrame;
uniform float iIntensity;

vec4 composite(vec4 under, vec4 over) {
    float a_out = over.a + under.a * (1. - over.a);
    return vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    gl_FragColor = texture2D(iFrame, uv);
    vec4 c;

    c = vec4(1., 1., 1., 1 - smoothstep(iIntensity - 0.1, iIntensity, length(uv - 0.5) / 0.5));
    gl_FragColor = composite(gl_FragColor, c);

    c = texture2D(iChannel[1], (uv - 0.5) / iIntensity + 0.5);
    c.a = 1. - smoothstep(iIntensity - 0.2, iIntensity - 0.1, length(uv - 0.5) / 0.5);
    gl_FragColor = composite(gl_FragColor, c);
}
