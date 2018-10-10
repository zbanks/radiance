import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

Item {
    property alias videoNode: vnr.videoNode

    Layout.fillWidth: true;
    Layout.fillHeight: true;
    Item {
        width: Math.min(parent.width, parent.height)
        height: width
        anchors.centerIn: parent;
        layer.enabled: true;

        CheckerboardBackground {
            anchors.fill: parent;
        }
        VideoNodePreview {
            id: vnr
            anchors.fill: parent
            previewAdapter: Globals.previewAdapter
        }
        BusyIndicator {
            id: busy
            anchors.fill: parent
            anchors.margins: 10
            opacity: vnr.videoNode && vnr.videoNode.nodeState == VideoNode.Loading ? 1 : 0
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
            opacity: vnr.videoNode && vnr.videoNode.nodeState == VideoNode.Broken ? 0.9 : 0
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
}

