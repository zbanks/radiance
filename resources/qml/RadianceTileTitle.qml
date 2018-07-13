import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Text {
    color: "#ddd"
    elide: Text.ElideMiddle
    style: Text.Raised
    styleColor: "#333"
    bottomPadding: 5
    topPadding: -3
    Rectangle {
        color: "#666"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        anchors.leftMargin: -3
        anchors.rightMargin: -3
        anchors.bottomMargin: 2
  }
}
