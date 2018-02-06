import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

Item {
    property var registry
    property var model
    property var context

    ColumnLayout {
        anchors.fill: parent

        TreeView {
            Layout.fillHeight: true;
            Layout.fillWidth: true;
            model: registry.library;
            backgroundVisible: false;
            headerVisible: false;
            alternatingRowColors: false

            TableViewColumn {
                role: "display"
                title: "Name"
            }
            style: TreeViewStyle {
                indentation: 30;
                branchDelegate: Text {
                    width: 30
                    height: 30
                    text: styleData.isExpanded ? "▼" : "▶"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "white"
                }
                frame: Item {}
            }
            itemDelegate: Item {
                Text {
                    color: "white"
                    text: styleData.value
                }
            }
            rowDelegate: Item {
                height: 30;
                
            }
        }
        // pass
        Action {
            id: nodeAddAction
            onTriggered: {
                var node = model.createVideoNode(nodeSelector.currentText);
                if (node && graph.lastClickedTile) {
                    graph.lastClickedTile.insertAfter(node);
                }
                model.flush();
                // TODO: This doesn't work because the view hasn't reloaded the graph yet
                //var tile = graph.view.tileForVideoNode(node);
                //graph.lastClickedTile = tile;
                //console.log("last tile", tile, node);
            }
        }
        Button {
            id: nodeAddButton
            text: "Add"
            action: nodeAddAction
        }
        Label {
            id: nodeAddDescription
            Layout.fillWidth: true;
            elide: Text.ElideRight;
            color: "#ddd";
            //text: NodeRegistry.nodeTypes[nodeSelector.currentText].description;
        }
    }
}
