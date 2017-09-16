import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;
    property alias intensity: slider.value;
    property alias slider: slider;
    property alias sliderGhost: sliderGhost;

    onVideoNodeChanged: {
        videoNode.intensity = Qt.binding(function() { return slider.value });
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.name : "(loading)";
            color: "#ddd";
        }
        
        /*
        ComboBox {
            Layout.fillWidth: true;
            model: NodeList.effectNames();
            editable: true;
        }
        */

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }
            VideoNodeRender {
                anchors.fill: parent;
                id: vnr;
                context: Globals.context;
                videoNodeId: tile.videoNode ? tile.videoNode.id : 0;
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

    Keys.onPressed: {
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
    }
}
