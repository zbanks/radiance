uniform lowp float iIntensity;
varying highp vec2 coords;
uniform sampler2D iChannelP;
uniform sampler2D iFrame;

void main() {
    vec4 c = texture2D(iFrame, 0.5 * (coords + 1.));
    gl_FragColor = c.bgra;
}
