import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

FocusScope {
    id: tile;
    property var videoNode;
    property int gridX;
    property int gridY;

    function sum(l) {
        var result = 0;
        for(var i=0; i<l.length; i++) result += l[i];
        return result;
    }

    width: 100;
    height: 170
    x: parent.width - (gridX + 1) * 100;
    y: gridY * 170;

    onVideoNodeChanged: {
    }

    RadianceTile {
        anchors.fill: parent;
        focus: true;
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            Layout.fillWidth: true;
            text: videoNode.imagePath;
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }

            VideoNodeRender {
                anchors.fill: parent;
                chain: 0;
                id: vnr;
                videoNode: tile.videoNode;
            }
        }

        /*
        ProgressBar {
            id: sliderGhost;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
        */
    }
}
