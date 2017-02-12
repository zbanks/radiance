uniform lowp float iIntensity;
varying highp vec2 coords;
uniform highp float iTime;
uniform sampler2D iChannelP;
uniform sampler2D iFrame;

// Alpha-compsite two colors, putting one on top of the other
vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
}

void main() {
    lowp float size = iIntensity * mod(iTime, 1.);
    gl_FragColor = vec4(1., 0., 0., 1. - smoothstep(size, size + .1, length(coords)));

    vec4 c = texture2D(iFrame, 0.5 * (coords + 1.));
    gl_FragColor = composite(c, gl_FragColor);
}
