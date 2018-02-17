import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQml.Models 2.2
import radiance 1.0

Item {
    id: libraryWidget
    property var registry
    property var model
    property var context

    ColumnLayout {
        anchors.fill: parent

        TreeView {
            id: librarytree

            /* Workaround for QTBUG-47243,
               which seems to still be present in
               Qt 5.10 */
            selection: ItemSelectionModel {
                id: selModel
                    model: librarytree.model
            }
            onClicked: {
                selModel.clearCurrentIndex();
                selModel.setCurrentIndex(index, 0x0002 | 0x0010);
            }
            /* End workaround */

            Layout.fillHeight: true;
            Layout.fillWidth: true;
            model: registry.library;
            backgroundVisible: false;
            headerVisible: false;
            alternatingRowColors: false
            horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff;
            verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff;

            TableViewColumn {
                role: "name"
            }
            style: TreeViewStyle {
                TextMetrics {
                    id: tm
                    text: "MM"
                }
                indentation: tm.width;
                branchDelegate: Text {
                    TextMetrics {
                        id: tm
                        text: "M"
                    }
                    width: tm.width
                    text: styleData.isExpanded ? "▼" : "▶"
                    color: "#aaa"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                frame: Item {}
            }
            itemDelegate: Item {
                id: container
                Text {
                    color: styleData.selected ? "white" : "#aaa"
                    font.bold: styleData.selected
                    text: styleData.value
                    verticalAlignment: Text.AlignVCenter
                }
            }
            rowDelegate: Item {
                height: tm.height;
                TextMetrics {
                    id: tm
                    text: " "
                }
            }
            onDoubleClicked: {
                var filename = model.data(index, Library.FileRole);
                var vn = registry.createFromFile(context, filename);
                if (vn) {
                    libraryWidget.model.addVideoNode(vn);
                    libraryWidget.model.flush();
                }
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
