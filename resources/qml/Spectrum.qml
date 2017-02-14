import QtQuick 2.3
import radiance 1.0

GraphicalDisplay {
    width: 500;
    height: 200;
    fragmentShader: "
        // Alpha-compsite two colors, putting one on top of the other
        vec4 composite(vec4 under, vec4 over) {
            float a_out = 1. - (1. - over.a) * (1. - under.a);
            return clamp(vec4((over.rgb * over.a  + under.rgb * under.a * (1. - over.a)) / a_out, a_out), vec4(0.), vec4(1.));
        }

        uniform sampler1D iSpectrum;
        uniform vec2 iResolution;

        void main(void) {
            vec2 uv = gl_FragCoord.xy / iResolution;
            float g = uv.y * 0.5 + 0.1;
            float w = 4.;

            gl_FragColor = vec4(0.);

            float freq = (uv.x - 0.5) + 0.5;
            float mag = (0.5 - uv.y) + 0.5;
            float d = (texture1D(iSpectrum, freq).r - mag) * 90.;
            float a = smoothstep(0., 1., d);
            float gb = 0.5 * clamp(0., 1., d / 30.);
            gl_FragColor = composite(gl_FragColor, vec4(1., gb, gb, a));
            gl_FragColor.rgb *= gl_FragColor.a;
        }"
}
