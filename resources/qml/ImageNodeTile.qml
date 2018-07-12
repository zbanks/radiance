import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

VideoNodeTile {
    id: tile;

    onVideoNodeChanged: {

    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        RadianceTileTitle {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.name : "";
        }

        CheckerboardPreview {
            videoNode: tile.videoNode
        }
    }

    Keys.onPressed: {

    }
}
