import QtQuick 2.3
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 500;
    implicitHeight: 100;

    fragmentShader: "
        #version 130
        // Alpha-compsite two colors, putting one on top of the other
        in vec2 uv;
        out vec4 fragcolor;

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

            fragcolor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = abs(uv.y - 0.5);
            vec4 wav = texture1D(iWaveform, freq);
            vec4 beats = texture1D(iBeats, freq);
            vec3 wf = (wav.rgb - mag) * 90.;
            wf = smoothstep(0., 1., wf);
            float level = (wav.a - mag) * 90.;
            level = (smoothstep(-5., 0., level) - smoothstep(0., 5., level));
            float beat = beats.x;
            fragcolor = composite(fragcolor, vec4(0.0, 0.0, 0.6, wf.b));
            fragcolor = composite(fragcolor, vec4(0.3, 0.3, 1.0, wf.g));
            fragcolor = composite(fragcolor, vec4(0.7, 0.7, 1.0, wf.r));
            fragcolor = composite(fragcolor, vec4(0.7, 0.7, 0.7, level * 0.5));
            fragcolor = composite(fragcolor, vec4(0.7, 0.7, 0.7, beat));
            fragcolor.rgb *= fragcolor.a;
        }"
}
