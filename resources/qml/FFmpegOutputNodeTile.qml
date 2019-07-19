import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    normalHeight: 200;
    normalWidth: 160;

    onVideoNodeChanged: {
        recordingCheck.checked = videoNode.recording;
        videoNode.recording = Qt.binding(function() { return recordingCheck.checked });
    }

    Connections {
        target: videoNode
        onRecordingChanged: {
            recordingCheck.checked = recording;
        }
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        RadianceTileTitle {
            text: "FFmpeg Output";
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
                videoNode: tile.videoNode;
            }
        }

        Label {
            Layout.fillWidth: true
            font.pixelSize: 14
            text: "Args:" + tile.videoNode.ffmpegArguments.join(" ")
            color: RadianceStyle.tileTextColor;
            elide: Text.ElideRight;
            wrapMode: Text.Wrap
        }
            
        CheckBox {
            id: recordingCheck
            text: "Rec"
        }
    }
}
