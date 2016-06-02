#version 130

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2D iChannel[2];
uniform sampler2D iFrame;
uniform float iIntensity;
uniform float iAudioHi;
uniform float iAudioMid;
uniform float iAudioLow;

vec4 composite(vec4 under, vec4 over) {
    float a_out = over.a + under.a * (1. - over.a);
    return vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out);
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;

    float factor = 1. - 10. * iIntensity * (iAudioLow - 0.3);
    factor = clamp(0.05, 2., factor);

    gl_FragColor = texture2D(iFrame, (uv - 0.5) * factor + 0.5);
}
