import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0
import "."

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

    ShaderEffect {
        id: dropRectangle
        property color highlightColor: RadianceStyle.dropTargetColor
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
            uniform vec4 highlightColor;
            void main() {
                float i = max(1. - length(2. * coord.xy - 1.), 0.);
                i = pow(i, 3.);
                vec4 c = highlightColor * i;
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

    onDropped: {
        if (drop.urls) {
            // Handle dropping of external content.
            // TODO: I am annoyed that this is handled in a separate place from
            // drag/drop of internal content (i.e. selected tiles) using largely duplicated code.
            // (That code is in dragDrop in VideoNodeTile.qml)
            var graph = parent.parent.graph;
            var model = parent.model;
            var url = drop.urls[0];
            if (url.startsWith("file://")) {
                url = url.slice(7);
            }
            var videoNode = graph.registry.createFromFile(defaultContext, url);
            model.addVideoNode(videoNode);
            if (videoNode) Qt.callLater(function() {
                var fn = fromNode;
                var tn = toNode;
                var ti = toInput;

                if (fn !== null && tn !== null) model.removeEdge(fn, tn, ti);

                if (fn !== null && videoNode.inputCount != 0) {
                    model.addEdge(fn, videoNode, 0);
                }
                if (tn !== null) {
                    model.addEdge(videoNode, tn, ti);
                }
                model.flush();
            });
        }
    }
}
