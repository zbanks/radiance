import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtQml.Models 2.2
import radiance 1.0
import "."

Item {
    id: consoleWidget
    property var graph
    property alias count: listModel.count
    signal popOut()
    signal popIn()

    function message(videoNode, str) {
        addItem("message", videoNode, str)
    }

    function warning(videoNode, str) {
        addItem("warning", videoNode, str)
    }

    function error(videoNode, str) {
        addItem("error", videoNode, str)
    }

    function addItem(type, videoNode, str) {
        listModel.append({"type": type, "videoNode": videoNode, "str": str});
        popOut();
    }

    ListView {
        id: listView
        clip: true

        anchors.fill: parent

        model: ListModel {
            id: listModel
        }
        delegate: Item {
            height: t.height + 8
            width: parent.width - 15
            anchors.margins: 5
            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                opacity: 0.1
                color: type == "warning" ? "gold" : type == "error" ? "red" : "green"
                radius: 3
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    graph.view.tileForVideoNode(videoNode).forceActiveFocus();
                }
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Text {
                    textFormat: Text.RichText
                    color: RadianceStyle.mainTextColor
                    id: t
                    text: str
                    Layout.maximumWidth: parent.width - 20
                    onLinkActivated: {
                        graph.view.tileForVideoNode(videoNode).consoleLinkClicked(link);
                    }
                }
            }
            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 10
                width: 12
                height: 12
                color: "transparent"
                border.color: RadianceStyle.mainLineColor
                border.width: 1
                Text {
                    text: "X"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: RadianceStyle.mainTextColor
                    anchors.centerIn: parent
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        listModel.remove(index)
                    }
                }
            }
        }

        onCountChanged: {
            if (count == 0) popIn();
        }
        onContentHeightChanged: {
            contentY = contentHeight - height;
        }

        ScrollIndicator.vertical: ScrollIndicator {}
    }
}
