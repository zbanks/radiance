import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

DropArea {
    id: dragTarget
    property alias dropProxy: dragTarget
    property VideoNode fromNode;
    property VideoNode toNode;
    property int toInput;
    property real gridX;
    property real gridY;
    property real gridHeight;
    property real posX;
    property real posY;

    property int padding: 5;
    property int blockWidth: 100;
    property int blockHeight: 170;

    width: blockWidth - 2 * padding
    height: (blockHeight - padding) * gridHeight - padding
    //x: (parent ? parent.width : 0) - (gridX + 1) * (blockWidth + padding)
    x: posX - width / 2
    y: gridY * (blockHeight + padding)

    keys: [ "videonode" ]

    ShaderEffect {
        id: dropRectangle
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
                vec4 c = vec4(1., 1., 0., 1.);
                float i = max(1. - length(2. * coord.xy - 1.), 0.);
                i = pow(i, 3.);
                c *= i;
                gl_FragColor = c * qt_Opacity;
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
