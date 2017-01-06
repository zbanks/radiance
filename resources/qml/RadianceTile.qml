import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Item {
    Rectangle {
        anchors.fill: parent;
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#222" }
            GradientStop { position: 1.0; color: "#111" }
        }
        radius: 10;
        border.width: 3;
        border.color: "#666";
    }
}
