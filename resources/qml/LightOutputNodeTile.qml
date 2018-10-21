import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    normalHeight: 260;
    normalWidth: 200;

    ColumnLayout {
        anchors.fill: parent;
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 5
        anchors.topMargin: 5

        RadianceTileTitle {
            Layout.fillWidth: true
            text: tile.videoNode.name ? tile.videoNode.name : "Light Output"
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 5
            layer.enabled: true

            LightOutputPreview {
                id: vnr
                videoNode: tile.videoNode
                anchors.fill: parent
            }
            BusyBrokenIndicator {
                anchors.fill: parent
                videoNode: vnr.videoNode
            }
        }
    }
    Keys.onPressed: {
        if (event.modifiers == Qt.NoModifier) {
            if (event.key == Qt.Key_R)
                videoNode.reload();
        }
    }
}
