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
    layer.enabled: true;

    id: rect;
    gradient: Gradient {
        GradientStop { position: 0.0; color: startColor; }
        GradientStop { position: 1.0; color: endColor; }
    }
    radius: 10;
    border.width: borderWidth;
    border.color: borderColor;
}
