import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

VideoNodeTile {
    id: tile;

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.videoPath : "";
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            VideoNodePreview {
                anchors.fill: parent;
                id: vnr;
                context: Globals.context;
                videoNodeId: tile.videoNode ? tile.videoNode.id : 0;
            }
        }

        Slider {
            id: slider
            enabled: tile.videoNode.duration > 0
            value: tile.videoNode.duration > 0 ? tile.videoNode.position / tile.videoNode.duration : 0
            state: pressed ? "seeking" : ""
            onValueChanged: {
                if (state == "seeking"
                    && tile.videoNode.duration > 0) {
                    tile.videoNode.position = tile.videoNode.duration * value;
                }
            }

            states: [
                State {
                    name: "seeking"
                    PropertyChanges {
                        target: slider;
                        value: value;
                    }
                }
            ]
        }
    }

    Keys.onPressed: {

    }
}
