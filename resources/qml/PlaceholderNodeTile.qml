import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        RowLayout {
            Label {
                Layout.fillWidth: true;
                text: "Placeholder";
                color: "#ddd";
                elide: Text.ElideRight;
            }
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            layer.enabled: true;

            CheckerboardBackground {
                anchors.fill: parent;
            }
            VideoNodePreview {
                id: vnr;
                anchors.fill: parent;
                previewAdapter: Globals.previewAdapter;
                videoNode: tile.videoNode;
            }
        }

        /*
        ComboBox {
            Layout.fillWidth: true;
            id: inputCountSelector;
            model: ["1 Input", "2 Inputs", "3 Inputs", "4 Inputs"];
            currentIndex: tile.videoNode ? tile.videoNode.inputCount - 1 : 0;
            onCurrentIndexChanged: {
                if (!tile.videoNode) return;
                tile.videoNode.inputCount = inputCountSelector.currentIndex + 1;
            }
        }
        */
    }
}
