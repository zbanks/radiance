import QtQuick 2.7
import QtQuick.Layouts 1.2
import radiance 1.0

VideoNodeTile {
    id: tile;

    normalHeight: 200;
    normalWidth: 160;

    function setFactorSelector() {
        for(var i = 0; i < factorSelector.model.count; ++i) {
            if (factorSelector.model.get(i).value == videoNode.factor) {
                factorSelector.currentIndex = i;
            }
        }
    }

    onVideoNodeChanged: {
        mutedCheck.checked = videoNode.mute;
        pausedCheck.checked = videoNode.pause;
        setFactorSelector();
        videoNode.mute = Qt.binding(function() { return mutedCheck.checked });
        videoNode.pause = Qt.binding(function() { return pausedCheck.checked });
        videoNode.factor = Qt.binding(function() { return factorSelector.model.get(factorSelector.currentIndex).value });
    }

    Connections {
        target: videoNode
        onMuteChanged: {
            mutedCheck.checked = mute;
        }
        onPauseChanged: {
            pausedCheck.checked = pause;
        }
        onFactorChanged: {
            setFactorSelector();
        }
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 10;

        RadianceTileTitle {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.name : "";
        }

        CheckerboardPreview {
            videoNode: tile.videoNode
        }

        Slider {
            property bool controlled;
            id: slider
            Layout.fillWidth: true
            from: 0
            to: tile.videoNode.duration
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

        ComboBox {
            id: factorSelector;
            Layout.fillWidth: true
            textRole: "text"
            model: ListModel {
                ListElement { text: "Crop"; value: MovieNode.Crop }
                ListElement { text: "Shrink"; value: MovieNode.Shrink }
                ListElement { text: "Zoom"; value: MovieNode.Zoom }
            }
            onCurrentIndexChanged: {
                videoNode.factor = model.get(currentIndex).value;
            }
        }

        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: mutedCheck
                text: "M"
            }

            CheckBox {
                id: pausedCheck
                text: "P"
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
