import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Item {
    property alias model: view.model
    property alias view: view
    property alias currentOutputName: viewWrapper.currentOutputName
    property alias lastClickedTile: viewWrapper.lastClickedTile
    Layout.fillWidth: true;
    Layout.fillHeight: true;

    property real zoomRate: 0.001
    property real zoomStep: 0.1
    property real zoomMin: 0.1
    property real zoomMax: 10
    property real intensityRate: 0.0003

    function insertVideoNode(videoNode) {
        model.addVideoNode(videoNode);
        if (lastClickedTile) {
            var edges = model.edges;
            for (var i=0; i<edges.length; i++) {
                if (edges[i].fromVertex == lastClickedTile.videoNode) {
                    model.addEdge(videoNode, edges[i].toVertex, edges[i].toInput);
                }
            }
            model.addEdge(lastClickedTile.videoNode, videoNode, 0);
        }
        model.flush();
        view.tileForVideoNode(videoNode).forceActiveFocus();
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: view.width * view.scale + 600;
        contentHeight: view.height * view.scale + 400;
        clip: true;

        Item {
            id: viewWrapper
            property var lastClickedTile
            property string currentOutputName: ""
            width: Math.max(view.width * view.scale + 400, flickable.width)
            height: Math.max(view.height * view.scale + 400, flickable.height)

            View {
                id: view
                model: model
                delegates: {
                    "EffectNode": "EffectNodeTile",
                    "ImageNode": "ImageNodeTile",
                    "MovieNode": "MovieNodeTile",
                    "ScreenOutputNode": "ScreenOutputNodeTile",
                    "FFmpegOutputNode": "FFmpegOutputNodeTile",
                    "PlaceholderNode": "PlaceholderNodeTile",
                    "": "VideoNodeTile"
                }
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2

                focus: true

                Behavior on x { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }
                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }

                // Temporary
                Keys.onPressed: {
                    if (event.key == Qt.Key_Space) {
                        view.Controls.changeControlRel(0, Controls.Enter, 1);
                    }
                }

                /*
                Rectangle {
                    opacity: 0.5
                    color: "red"
                    anchors.fill: parent
                }
                //*/
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    view.select([]);
                    view.focus = true;
                }
                z: -1;

                onWheel: {
                    wheel.accepted = false;
                    if (wheel.modifiers & Qt.ControlModifier) {
                        var scale = view.scale;
                        scale = Math.max(Math.min(scale * Math.exp(wheel.angleDelta.y * zoomRate), zoomMax), zoomMin);
                        view.scale = scale;
                        wheel.accepted = true;
                    } else if (!view.focus) {
                        view.Controls.changeControlRel(0, Controls.PrimaryParameter, wheel.angleDelta.y * intensityRate);
                        wheel.accepted = true;
                    }
                }
            }
            Action {
                id: zoomIn
                text: "Zoom &In"
                shortcut: StandardKey.ZoomIn
                onTriggered: {
                    view.scale *= Math.exp(zoomStep);
                }
            }
            Action {
                id: zoomOut
                text: "Zoom &Out"
                shortcut: StandardKey.ZoomOut
                onTriggered: {
                    view.scale *= Math.exp(-zoomStep);
                }
            }
            Action {
                id: resetZoom
                text: "&Reset zoom"
                shortcut: "Ctrl+0"
                onTriggered: {
                    view.scale = 1;
                }
            }
        }
    }
}
