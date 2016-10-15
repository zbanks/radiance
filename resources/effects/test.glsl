uniform lowp float iIntensity;
varying highp vec2 coords;
void main() {
    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));
    i = smoothstep(iIntensity - 0.8, iIntensity + 0.8, i);
    //i = floor(i * 20.) / 20.;
    gl_FragColor = vec4(coords * .5 + .5, i, i);
}
