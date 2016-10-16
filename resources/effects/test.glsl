uniform lowp float iIntensity;
varying highp vec2 coords;
uniform sampler2D iChannelP;
uniform sampler2D iFrame;

// Alpha-compsite two colors, putting one on top of the other
vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
}

void main() {
    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));
    i = smoothstep(iIntensity - 0.8, iIntensity + 0.8, i);
    //i = floor(i * 20.) / 20.;
    gl_FragColor = vec4(coords * .5 + .5, i, i);

    vec4 c = texture2D(iFrame, 0.5 * (coords + 1.));
    gl_FragColor = composite(c, gl_FragColor);
}
