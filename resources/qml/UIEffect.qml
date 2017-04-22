import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

FocusScope {
    id: tile;
    property alias effect: effect;
    implicitWidth: 150;
    implicitHeight: 300;

    RadianceTile {
        anchors.fill: parent;
        focus: true;
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

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            text: effect.source;
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            Effect {
                id: effect;
                anchors.fill: parent;
                intensity: slider.value;
            }
        }

        Slider {
            id: slider;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
    }
}
