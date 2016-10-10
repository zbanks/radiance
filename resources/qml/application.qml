import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 1.4
import radiance 1.0

ColumnLayout {
    Rectangle {
        width: 100;
        height: 100;
        color: "red";
    }

    Button {
        text: "Disappointment";
    }

    Item {
        Effect {
            intensity: slider.value;
        }

        Slider {
            id: slider;
            minimumValue: 0;
            maximumValue: 1;
        }
    }
}
