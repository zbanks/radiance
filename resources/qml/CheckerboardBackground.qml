import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4

ShaderEffect {
    vertexShader: "
        uniform highp mat4 qt_Matrix;
        attribute highp vec4 qt_Vertex;
        attribute highp vec2 qt_MultiTexCoord0;
        varying highp vec2 coord;
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }"
    fragmentShader: "
        varying highp vec2 coord;
        uniform lowp float qt_Opacity;
        void main() {
            vec2 qc = floor(coord.xy * 12.);
            vec3 c = vec3(0.2) + vec3(0.15) * mod(qc.x + qc.y, 2.);
            gl_FragColor = vec4(c, 1) * qt_Opacity;
        }"
}
