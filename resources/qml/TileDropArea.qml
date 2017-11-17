import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

DropArea {
    id: dragTarget
    property alias dropProxy: dragTarget
    property VideoNode fromNode
    property VideoNode toNode
    property int toInput
    property real gridX
    property real gridY
    property real gridHeight
    property real posX
    property real posY
    property real posHeight: 170

    property int padding: 5
    property int blockWidth: 100
    property int blockHeight: 170

    width: blockWidth
    height: posHeight
    x: posX - width / 2
    y: posY

    keys: [ "videonode" ]

    ShaderEffect {
        id: dropRectangle
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
                vec4 c = vec4(1., 1., 0., 1.);
                float i = max(1. - length(2. * coord.xy - 1.), 0.);
                i = pow(i, 3.);
                c *= i;
                fragColor = c * qt_Opacity;
            }"

        visible: false

        anchors.fill: parent

        states: [
            State {
                when: dragTarget.containsDrag
                PropertyChanges {
                    target: dropRectangle
                    visible: true
                }
            }
        ]
    }
}
