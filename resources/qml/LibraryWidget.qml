import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import radiance 1.0

RowLayout {
    property var model
    property var graph
    function nodeAddAction() {
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

    ComboBox {
        id: nodeSelector;
        model: Object.keys(NodeRegistry.nodeTypes);
        editable: true;
        Layout.preferredWidth: 200;
        onAccepted: nodeAddAction()
    }
    ToolButton {
        id: nodeAddButton
        text: "Add"
        onClicked: nodeAddAction()
    }
    ToolButton {
        id: nodeRegistryReload
        text: "Reload Library"
        onClicked: {
            NodeRegistry.reload();
        }
    }
}
