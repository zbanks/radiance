import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    normalHeight: 400;
    normalWidth: 300;

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        RowLayout {
            Label {
                Layout.fillWidth: true;
                text: "ffmpeg Output";
                color: "#ddd";
                elide: Text.ElideMiddle;
            }
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }
            VideoNodePreview {
                id: vnr;
                anchors.fill: parent;
                previewAdapter: Globals.previewAdapter;
                videoNodeId: tile.videoNode ? tile.videoNode.id : 0;
            }
        }

        Label {
            Layout.fillWidth: true
            text: "Args:" + tile.videoNode.ffmpegArguments.join(" ")
            color: "#ddd";
            elide: Text.ElideMiddle;
        }
            
        CheckBox {
            id: recordingCheck
            checked: tile.videoNode ? tile.videoNode.recording : false;

            onCheckedChanged: {
                tile.videoNode.recording = checked;
            }

            style: CheckBoxStyle {
                label: Text {
                    color: "#ddd"
                    text: "Recording"
                }
            }
        }
    }
}
