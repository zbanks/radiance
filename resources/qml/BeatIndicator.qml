import QtQuick 2.7
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 65;
    implicitHeight: 65;

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        uniform highp float iTime;

        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return vec4((over.rgb + under.rgb * (1. - over.a)), a_out);
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
            vec4 ballColor = mix(vec4(0.6, 0., 0., 1.), vec4(0.8, 0.5, 0.5, 1.), clamp(10. * (ballLoc.y - uv.y), 0., 1.));
            vec4 ball = ballColor * (1. - step(0.1, length(uv - ballLoc)));

            float floorBox = box((uv - vec2(0.2, 0.9)) / vec2(0.6, 0.05));
            vec4 floorColor = mix(vec4(0., 0., 0., 1.), vec4(0.6, 0.6, 0.6, 1.), clamp(20. * (0.925 - uv.y), 0., 1.));
            vec4 floor = floorColor * floorBox;

            fragColor = composite(ball, floor);
        }"
}
