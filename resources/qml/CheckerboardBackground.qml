import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4

ShaderEffect {
    vertexShader: "
        #version 150
        uniform mat4 qt_Matrix;
        in vec4 qt_Vertex;
        in vec2 qt_MultiTexCoord0;
        out vec2 coord;
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }"
    fragmentShader: "
        #version 150
        in vec2 coord;
        uniform float qt_Opacity;
        out vec4 fragColor;
        void main() {
            vec2 qc = floor(coord.xy * 12.);
            vec3 c = vec3(0.2) + vec3(0.15) * mod(qc.x + qc.y, 2.);
            fragColor = vec4(c, 1) * qt_Opacity;
        }"
}
