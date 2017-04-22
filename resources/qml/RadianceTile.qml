import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

Rectangle {
    property color startColor: activeFocus ? "#113" : "#111";
    property color endColor: activeFocus ? "#181838" : "#181818";
    property real borderWidth: 3;
    property color borderColor: "#666";
    property var slider;
    layer.enabled: true;

    id: rect;
    gradient: Gradient {
        GradientStop { position: 0.0; color: startColor; }
        GradientStop { position: 1.0; color: endColor; }
    }
    radius: 10;
    border.width: borderWidth;
    border.color: borderColor;

    Keys.onPressed: {
        if (slider) {
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
}
