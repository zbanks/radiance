import QtQuick 2.3
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 25;
    implicitHeight: 100;

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        uniform highp float iTime;

        void main(void) {
            vec2 xy = gl_FragCoord.xy;
            float t = mod(iTime, 1.0);
            vec2 center = vec2(12.5, 40 + pow(t - 0.5, 2) * 4 * 50);
            float R = 6;
            float d = distance(xy, center);
            fragColor = vec4(1.0, 0., 0., 1.0) * step(d, R) + vec4(0.7, 0.7, 0.7, 1.0) * step(90 + R, xy.y);
        }"
}
