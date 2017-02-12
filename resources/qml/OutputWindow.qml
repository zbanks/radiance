import QtQuick 2.3
import QtQuick.Controls 1.2
import radiance 1.0

ApplicationWindow {
    id: window
    property var source;

    Item {
        anchors.fill: parent;

        Rectangle {
            anchors.fill: parent;
            color: "black";
        }
        Output {
            anchors.fill: parent;
            source: window.source;
        }
    }
}
