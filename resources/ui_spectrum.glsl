#version 130

uniform vec2 iResolution;
uniform float iBins;
uniform sampler1D iSpectrum;

float rounded_rect_df(vec2 center, vec2 size, float radius) {
    return length(max(abs(gl_FragCoord.xy - center) - size, 0.0)) - radius;
}

vec4 composite(vec4 under, vec4 over) {
    return vec4(over.rgb * over.a  + under.rgb * under.a * (1. - over.a), over.a + under.a * (1. - over.a));
}

void main(void) {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float g = uv.y * 0.5 + 0.1;
    float w = 4.;

    gl_FragColor = vec4(0.);

    float df = max(rounded_rect_df(vec2(200., 100.), vec2(165., 65.), 25.), 0.);

    float shrink_freq = 190. / 200.;
    float shrink_mag = 90. / 100.;
    float freq = (uv.x - 0.5) / shrink_freq + 0.5;
    float mag = (uv.y - 0.5) * shrink_mag + 0.5;
    float d = (texture1D(iSpectrum, freq).r - mag) * 90.;
    float a = smoothstep(0., 1., d) * (1. - step(1., df));
    float gb = 0.5 * clamp(0., 1., d / 30.);
    gl_FragColor = composite(gl_FragColor, vec4(1., gb, gb, a));
    gl_FragColor = composite(gl_FragColor, vec4(0.3, 0.3, 0.3, smoothstep(0., 1., df) - smoothstep(2., 5., df)));
}
