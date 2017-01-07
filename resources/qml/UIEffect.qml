import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RadianceTile {
    id: tile;
    property alias effect: effect;
    implicitWidth: 200;
    implicitHeight: 300;

    Keys.onPressed: {
        if (event.key == Qt.Key_J)
            slider.value -= 0.1;
        else if (event.key == Qt.Key_K)
            slider.value += 0.1;
    }

    Drag.active: dragArea.drag.active;

    MouseArea {
        id: dragArea;
        anchors.fill: parent;
        onClicked: {
            // TODO reparent to draw on top of everything
            tile.focus = true;
        }
        onReleased: {
            // TODO reparent if dropped somewhere relevant
            tile.x = 0;
            tile.y = 0;
        }
        drag.target: parent;
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
