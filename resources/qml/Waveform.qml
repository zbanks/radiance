import QtQuick 2.3
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 500;
    implicitHeight: 100;

    fragmentShader: "
        #version 150

        in vec2 uv;
        out vec4 fragColor;

        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
        }

        uniform sampler1D iWaveform;
        uniform sampler1D iBeats;
        uniform vec2 iResolution;

        void main(void) {
            float g = uv.y * 0.5 + 0.1;
            float w = 4.;

            fragColor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = abs(uv.y - 0.5);
            vec4 wav = texture(iWaveform, freq);
            vec4 beats = texture(iBeats, freq);
            vec3 wf = (wav.rgb - mag) * 90.;
            wf = smoothstep(0., 1., wf);
            float level = (wav.a - mag) * 90.;
            level = (smoothstep(-5., 0., level) - smoothstep(0., 5., level));
            float beat = beats.x;
            fragColor = composite(fragColor, vec4(0.0, 0.0, 0.6, wf.b));
            fragColor = composite(fragColor, vec4(0.3, 0.3, 1.0, wf.g));
            fragColor = composite(fragColor, vec4(0.7, 0.7, 1.0, wf.r));
            fragColor = composite(fragColor, vec4(0.7, 0.7, 0.7, level * 0.5));
            fragColor = composite(fragColor, vec4(0.7, 0.7, 0.7, beat));
            fragColor.rgb *= fragColor.a;
        }"
}
