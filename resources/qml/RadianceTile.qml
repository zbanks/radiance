import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Rectangle {
    property bool selected: false;
    property color startColor: selected ? "#116" : "#111";
    property color endColor: selected ? "#181868" : "#181818";
    property real borderWidth: 3;
    property color borderColor: activeFocus ? "#A92" : "#666";
    property int padding: 5;
    property int blockWidth: 100;
    property int blockHeight: 170;
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
