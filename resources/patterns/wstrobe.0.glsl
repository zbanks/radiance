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

    float freq;
    if(iIntensity < 0.05) freq = 0.;
    else if(iIntensity < 0.15) freq = 4.;
    else if(iIntensity < 0.25) freq = 2.;
    else if(iIntensity < 0.35) freq = 1.;
    else if(iIntensity < 0.45) freq = 0.5;
    else if(iIntensity < 0.55) freq = 0.25;
    else if(iIntensity < 0.65) freq = 0.125;
    else if(iIntensity < 0.75) freq = 0.0625;
    else freq = 0.03125;

    if(freq > 0) {
        c = vec4(1., 1., 1., 1. - mod(iTime, freq) / freq);
        gl_FragColor = composite(gl_FragColor, c);
    }
}
