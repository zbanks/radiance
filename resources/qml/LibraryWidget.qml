import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

GroupBox {
    property var model
    property var graph
    ColumnLayout {
        RowLayout {
            ComboBox {
                id: nodeSelector;
                model: Object.keys(NodeRegistry.nodeTypes);
                editable: true;
                Layout.preferredWidth: 200;
                onAccepted: nodeAddAction.trigger()
            }
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
            Button {
                id: nodeRegistryReload
                text: "Reload Library"
                onClicked: {
                    NodeRegistry.reload();
                }
            }
        }
        Label {
            id: nodeAddDescription
            Layout.fillWidth: true;
            elide: Text.ElideRight;
            color: "#ddd";
            text: NodeRegistry.nodeTypes[nodeSelector.currentText].description;
        }
    }
}
