import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

ColumnLayout {
    Component.onCompleted: UISettings.previewSize = "300x300";

    Rectangle {
        width: 100;
        height: 100;
        color: "red";
    }

    Button {
        text: "Disappointment";
    }

    Item {
        width: 300;
        height: 200;

        Effect {
            anchors.fill: parent;
            intensity: slider.value;
        }

        Slider {
            id: slider;
            minimumValue: 0;
            maximumValue: 1;
        }
    }
}
