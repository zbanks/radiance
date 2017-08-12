import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

FocusScope {
    id: tile;
    property var videoNode;
    property alias intensity: slider.value;
    property alias slider: slider;
    property alias sliderGhost: sliderGhost;

    width: 100;
    height: 170;

    onVideoNodeChanged: {
        videoNode.intensity = Qt.binding(function() { return slider.value });
    }

    RadianceTile {
        anchors.fill: parent;
        focus: true;
        slider: slider;
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode.name;
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            VideoNodeRender {
                anchors.fill: parent;
                chain: 0;
                id: vnr;
                videoNode: tile.videoNode;
            }
        }

        Slider {
            id: slider;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }

        /*
        ProgressBar {
            id: sliderGhost;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
        */

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
    }
}
