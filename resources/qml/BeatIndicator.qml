import QtQuick 2.7
import radiance 1.0
import "."

GraphicalDisplay {
    implicitWidth: 65;
    implicitHeight: 65;

    property color ballColorBottom: RadianceStyle.ballColor
    property color ballColorTop: Qt.lighter(ballColorBottom, 1.5)
    property color ballOutlineColor: Qt.darker(ballColorBottom, 1.5)
    property color floorColor: RadianceStyle.mainLineColor

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        uniform highp float iTime;
        uniform vec4 ballColorBottom;
        uniform vec4 ballColorTop;
        uniform vec4 ballOutlineColor;
        uniform vec4 floorColor;

        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb + under.rgb * (1. - over.a)), a_out), vec4(0.), vec4(1.));
        }

        // Box from [0, 0] to (1, 1)
        float box(vec2 p) {
            vec2 b = step(0., p) - step(1., p);
            return b.x * b.y;
        }

        void main(void) {
            float height = 1. - pow(abs(2. * (mod(iTime, 1.) - 0.5)), 2.);
            height *= (1. - 0.4 * (1. - step(3., mod(iTime, 4.))));

            vec2 ballLoc = vec2(0.5, 0.8 - 0.6 * height);
            vec4 ballColor = mix(ballColorBottom, ballColorTop, clamp(10. * (ballLoc.y - uv.y), 0., 1.));
            float ball = 1. - smoothstep(0.08, 0.09, length(uv - ballLoc));
            float ballOutline = 1. - smoothstep(0.09, 0.1, length(uv - ballLoc));
            float floorBox = box((uv - vec2(0.2, 0.9)) / vec2(0.6, 0.05));

            fragColor = vec4(0.);
            fragColor = composite(fragColor, floorBox * floorColor);
            fragColor = composite(fragColor, ballOutline * ballOutlineColor);
            fragColor = composite(fragColor, ball * ballColor);
        }"
}
