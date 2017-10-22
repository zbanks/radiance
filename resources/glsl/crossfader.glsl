uniform lowp float iParameter;
varying highp vec2 coords;
uniform sampler2D iLeft;
uniform sampler2D iRight;

// Alpha-compsite two colors, putting one on top of the other
vec4 composite(vec4 under, vec4 over) {
    float a_out = 1. - (1. - over.a) * (1. - under.a);
    return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
}

void main() {
    vec4 l = texture(iLeft, 0.5 * (coords + 1.));
    vec4 r = texture(iRight, 0.5 * (coords + 1.));
    gl_FragColor = l * (1. - iParameter) + r * iParameter;
}
