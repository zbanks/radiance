import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;
    property alias intensity: slider.value;
    property alias slider: slider;
    //property alias sliderGhost: sliderGhost;
    property int attachedParameter: -1

    onVideoNodeChanged: {
        tile.intensity = videoNode.intensity;
        frequencyCombo.currentIndex = frequencyCombo.find(videoNode.frequency + "");
        videoNode.intensity = Qt.binding(function() { return slider.value });
        videoNode.frequency = Qt.binding(function() { return frequencyCombo.currentText });
    }

    Connections {
        target: videoNode
        onIntensityChanged: {
            slider.value = intensity;
        }
        onFrequencyChanged: {
            frequencyCombo.currentIndex = frequencyCombo.find(frequency + "");
        }
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;
        spacing: 5;

        RowLayout {
            RadianceTileTitle {
                Layout.fillWidth: true;
                text: videoNode ? videoNode.name : "(loading)";
            }
        
            Label {
                text: attachedParameter >= 0 ? "#" + attachedParameter : "";
                color: "#ddd";
            }
        }

        CheckerboardPreview {
            videoNode: tile.videoNode
        }

        /*
        ComboBox {
            Layout.fillWidth: true;
            model: NodeList.effectNames();
            editable: true;
        }
        */

        RowLayout {
            Slider {
                id: slider;
                Layout.fillWidth: true;
                minimumValue: 0;
                maximumValue: 1;

                style: SliderStyle {
                    handle: Rectangle {
                        height: 20
                        width: 0
                        radius: width/2
                        color: "transparent"
                        border.width: 0
                        border.color: "white"
                    }

                    groove: Rectangle {
                        implicitHeight: 5
                        radius: height/2
                        border.color: "white"
                        color: "transparent"
                        Rectangle {
                            height: parent.height
                            width: styleData.handlePosition
                            radius: height/2
                            color: "white"
                        }
                    }
                }
            }

            ComboBox {
                id: frequencyCombo;
                model: [0, 0.25, 0.5, 1, 2, 4, 8, 16, 32]
                style: ComboBoxStyle {
                    id: comboBox
                    background: Rectangle {
                        id: rectCategory
                        implicitWidth: textMetrics.width * 4
                        implicitHeight: textMetrics.height * 1.3
                        color: "transparent"
                        border.width: 1
                        border.color: "white"
                        radius: 15
                        TextMetrics {
                            id: textMetrics
                            text: "X"
                            font.pointSize: 6
                        }
                    }
                    label: Text {
                        id: labelText
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                        text: control.currentText
                        font.pointSize: 6
                    }
                }
            }
        }

        /*
        ProgressBar {
            id: sliderGhost;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
        */

        /* No MIDI indicator until we actually need it
        Slider {
            // TODO: How do we make this indicator-only?
            id: sliderGhost;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
            style: SliderStyle {
                handle: Rectangle {}
            }
        }
        */
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.NoModifier) {
            if (event.key == Qt.Key_J)
                slider.value -= 0.1;
            else if (event.key == Qt.Key_K)
                slider.value += 0.1;
            else if (event.key == Qt.Key_QuoteLeft)
                slider.value = 0.0;
            else if (event.key == Qt.Key_1)
                slider.value = 0.1;
            else if (event.key == Qt.Key_2)
                slider.value = 0.2;
            else if (event.key == Qt.Key_3)
                slider.value = 0.3;
            else if (event.key == Qt.Key_4)
                slider.value = 0.4;
            else if (event.key == Qt.Key_5)
                slider.value = 0.5;
            else if (event.key == Qt.Key_6)
                slider.value = 0.6;
            else if (event.key == Qt.Key_7)
                slider.value = 0.7;
            else if (event.key == Qt.Key_8)
                slider.value = 0.8;
            else if (event.key == Qt.Key_9)
                slider.value = 0.9;
            else if (event.key == Qt.Key_0)
                slider.value = 1.0;
            else if (event.key == Qt.Key_R)
                videoNode.reload();
        } else if (event.modifiers == Qt.ControlModifier) {
            if (event.key == Qt.Key_QuoteLeft)
                attachedParameter = -1;
            else if (event.key == Qt.Key_1)
                attachedParameter = 1;
            else if (event.key == Qt.Key_2)
                attachedParameter = 2;
            else if (event.key == Qt.Key_3)
                attachedParameter = 3;
            else if (event.key == Qt.Key_4)
                attachedParameter = 4;
            else if (event.key == Qt.Key_5)
                attachedParameter = 5;
            else if (event.key == Qt.Key_6)
                attachedParameter = 6;
            else if (event.key == Qt.Key_7)
                attachedParameter = 7;
            else if (event.key == Qt.Key_8)
                attachedParameter = 8;
            else if (event.key == Qt.Key_9)
                attachedParameter = 9;
            else if (event.key == Qt.Key_0)
                attachedParameter = 0;
        }
    }

    Controls.onControlChangedAbs: {
        if (control == Controls.PrimaryParameter) {
            intensity = value; // TODO soft takeover
        }
        if (control >= Controls.Parameter0 && control <= Controls.Parameter9
         && control - Controls.Parameter0 == attachedParameter) {
            intensity = value;
        }
        if (control == Controls.Frequency) {
            videoNode.frequency = value;
        }
    }

    Controls.onControlChangedRel: {
        if (control == Controls.PrimaryParameter) {
            var i = intensity + value;
            if (i > 1) i = 1;
            if (i < 0) i = 0;
            intensity = i;
        }
        if (control >= Controls.Parameter0 && control <= Controls.Parameter9
         && control - Controls.Parameter0 == attachedParameter) {
            var i = intensity + value;
            if (i > 1) i = 1;
            if (i < 0) i = 0;
            intensity = i;
        }
    }
}
