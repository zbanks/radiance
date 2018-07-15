import QtQuick 2.7
import QtQuick.Window 2.2
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 330;
    implicitHeight: 65;

    property color lowColor: Qt.darker(midColor, 1.5)
    property color midColor: RadianceStyle.waveformColor
    property color hiColor: Qt.lighter(midColor, 1.5)
    property color levelColor: RadianceStyle.mainLineColor

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb + under.rgb * (1. - over.a)), a_out), vec4(0.), vec4(1.));
        }

        uniform sampler1D iWaveform;
        uniform sampler1D iBeats;
        uniform vec2 iResolution;
        uniform vec4 lowColor;
        uniform vec4 midColor;
        uniform vec4 hiColor;
        uniform vec4 levelColor;

        void main(void) {
            float g = uv.y * 0.5 + 0.1;
            float w = 4.;

            fragColor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = abs(uv.y - 0.5);
            vec4 wav = texture(iWaveform, freq);
            vec4 beats = texture(iBeats, freq);
            vec3 wfDist = (wav.rgb - mag) * 90.;
            vec3 wf = smoothstep(0., 1., wfDist);
            float levelDist = (wav.a - mag) * 90.;
            float thickness = 1.;
            float level = smoothstep(-2., -1., levelDist);
            float beat = (1. - beats.x) * smoothstep(0., 1., max(wfDist.b, max(wfDist.g, wfDist.r)));
            fragColor = composite(fragColor, levelColor * level);
            fragColor = composite(fragColor, lowColor * wf.b);
            fragColor = composite(fragColor, midColor * wf.g);
            fragColor = composite(fragColor, hiColor * wf.r);
            fragColor = composite(fragColor, vec4(vec3(0.), 0.5 * beat));
        }"
}
