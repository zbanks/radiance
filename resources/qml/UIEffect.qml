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
        slider: slider;
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
