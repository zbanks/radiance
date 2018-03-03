import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    normalHeight: 400;
    normalWidth: 300;

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        RowLayout {
            Label {
                Layout.fillWidth: true;
                text: "Screen Output";
                color: "#ddd";
                elide: Text.ElideMiddle;
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

        ComboBox {
            id: screenSelector;
            model: tile.videoNode ? tile.videoNode.availableScreens : null;
            onModelChanged: {
                if (!tile.videoNode) return;
                var i = model.indexOf(tile.videoNode.screenName);
                if (i >= 0) {
                    screenSelector.currentIndex = i;
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: visibleCheck
                checked: tile.videoNode ? tile.videoNode.shown : false;

                Connections {
                    target: tile.videoNode
                    onShownChanged: {
                        visibleCheck.checked = shown;
                    }
                }

                onCheckedChanged: {
                    if (checked) {
                        tile.videoNode.screenName = screenSelector.currentText;
                    }
                    tile.videoNode.shown = checked;
                }

                style: CheckBoxStyle {
                    label: Text {
                        color: "#ddd"
                        text: "Visible"
                    }
                }
            }
        }
    }
}
