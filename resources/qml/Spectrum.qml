import QtQuick 2.7
import QtQuick.Window 2.2
import radiance 1.0
import "."

GraphicalDisplay {
    implicitWidth: 330;
    implicitHeight: 65;

    property color spectrumColorTop: Qt.lighter(spectrumColorBottom, 1.75)
    property color spectrumColorBottom: RadianceStyle.spectrumColor
    property color spectrumColorOutline: Qt.darker(spectrumColorBottom, 2.)

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb + under.rgb * (1. - over.a)), a_out), vec4(0.), vec4(1.));
        }

        uniform sampler1D iSpectrum;
        uniform vec2 iResolution;
        uniform vec4 spectrumColorTop;
        uniform vec4 spectrumColorBottom;
        uniform vec4 spectrumColorOutline;

        void main(void) {
            float g = uv.y * 0.5 + 0.1;
            float w = 4.;

            fragColor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = (0.5 - uv.y) + 0.5;
            float h = texture(iSpectrum, freq).r;
            float smoothEdge = 0.04;
            h *= smoothstep(0., smoothEdge, freq) - smoothstep(1. - smoothEdge, 1., freq);
            float d = (h - mag) * 90.;
            fragColor = composite(fragColor, mix(spectrumColorTop, spectrumColorBottom, clamp(d / 30., 0., 1.)) * step(1., d));
            fragColor = composite(fragColor, spectrumColorOutline * (smoothstep(0., 1., d) - smoothstep(3., 4., d) ));
        }"
}
