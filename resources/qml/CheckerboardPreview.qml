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
            running: vnr.videoNode && vnr.videoNode.nodeState == VideoNode.Loading
        }
    }
}

