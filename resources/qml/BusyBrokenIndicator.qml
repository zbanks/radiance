import QtQuick 2.7
import QtQuick.Controls 1.4
import radiance 1.0
import "."

Item {
    property VideoNode videoNode;

    BusyIndicator {
        id: busy
        anchors.fill: parent
        anchors.margins: 10
        opacity: videoNode && videoNode.nodeState == VideoNode.Loading ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                easing {
                    type: Easing.InOutQuad
                    amplitude: 1.0
                    period: 0.5
                }
                duration: 500
            }
        }
    }
    Rectangle {
        anchors.fill: parent
        color: RadianceStyle.tileLineColor
        opacity: videoNode && videoNode.nodeState == VideoNode.Broken ? 0.9 : 0
        Behavior on opacity {
            NumberAnimation {
                easing {
                    type: Easing.InOutQuad
                    amplitude: 1.0
                    period: 0.5
                }
                duration: 500
            }
        }
        Text {
            id: bang
            text: "!"
            font.pixelSize: parent.height * 0.9
            color: RadianceStyle.accent
            anchors.centerIn: parent
        }
    }
}
