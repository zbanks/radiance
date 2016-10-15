uniform lowp float t;
varying highp vec2 coords;
void main() {
    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));
    i = smoothstep(t - 0.8, t + 0.8, i);
    gl_FragColor = vec4(coords * .5 + .5, 0.0, 1.0).xzyw;
}
