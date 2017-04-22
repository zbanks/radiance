import QtQuick 2.3
import radiance 1.0

GraphicalDisplay {
    implicitWidth: 300;
    implicitHeight: 100;

    fragmentShader: "
        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
        }

        uniform sampler1D iWaveform;
        uniform sampler1D iBeats;
        uniform vec2 iResolution;

        void main(void) {
            vec2 uv = gl_FragCoord.xy / iResolution;
            float g = uv.y * 0.5 + 0.1;
            float w = 4.;

            gl_FragColor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = abs(uv.y - 0.5);
            vec4 wav = texture1D(iWaveform, freq);
            vec4 beats = texture1D(iBeats, freq);
            vec3 wf = (wav.rgb - mag) * 90.;
            wf = smoothstep(0., 1., wf);
            float level = (wav.a - mag) * 90.;
            level = (smoothstep(-5., 0., level) - smoothstep(0., 5., level));
            float beat = beats.x;
            gl_FragColor = composite(gl_FragColor, vec4(0.0, 0.0, 0.6, wf.b));
            gl_FragColor = composite(gl_FragColor, vec4(0.3, 0.3, 1.0, wf.g));
            gl_FragColor = composite(gl_FragColor, vec4(0.7, 0.7, 1.0, wf.r));
            gl_FragColor = composite(gl_FragColor, vec4(0.7, 0.7, 0.7, level * 0.5));
            gl_FragColor = composite(gl_FragColor, vec4(0.7, 0.7, 0.7, beat));
            gl_FragColor.rgb *= gl_FragColor.a;
        }"
}
