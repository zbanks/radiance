import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

VideoNodeTile {
    id: tile;

    onVideoNodeChanged: {
        mutedCheck.checked = videoNode.mute;
        pausedCheck.checked = videoNode.pause;
        videoNode.mute = Qt.binding(function() { return mutedCheck.checked });
        videoNode.pause = Qt.binding(function() { return pausedCheck.checked });
    }

    Connections {
        target: videoNode
        onMuteChanged: {
            mutedCheck.checked = mute;
        }
        onPauseChanged: {
            pausedCheck.checked = pause;
        }
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.name : "";
            color: "#ddd";
            elide: Text.ElideMiddle;
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
                previewAdapter: Globals.previewAdapter;
                videoNode: tile.videoNode;
            }
        }

        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: mutedCheck
            }

            CheckBox {
                id: pausedCheck
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Slider {
                property bool controlled;
                id: slider
                Layout.fillWidth: true
                minimumValue: 0
                maximumValue: tile.videoNode.duration
                enabled: tile.videoNode.duration > 0
                value: tile.videoNode.duration > 0 ? tile.videoNode.position : 0
                state: (pressed || controlled) ? "seeking" : ""
                onValueChanged: {
                    if (state == "seeking"
                        && tile.videoNode.duration > 0) {
                        tile.videoNode.position = value;
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
    }

    Timer {
        id: expireControl
        interval: 300; running: false; repeat: false
        onTriggered: {
            slider.controlled = false;
        }
    }

    Controls.onControlChangedRel: {
        if (control == Controls.PrimaryParameter) {
            slider.controlled = true;
            slider.value += value * 10;
            expireControl.restart();
        }
    }
}
