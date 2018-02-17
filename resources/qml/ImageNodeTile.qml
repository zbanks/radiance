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

        Label {
            Layout.fillWidth: true;
            text: videoNode ? videoNode.name : "";
            color: "#ddd";
            elide: Text.ElideMiddle;
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            VideoNodePreview {
                anchors.fill: parent;
                id: vnr;
                previewAdapter: Globals.previewAdapter;
                videoNodeId: tile.videoNode ? tile.videoNode.id : 0;
            }
        }
    }

    Keys.onPressed: {

    }
}
