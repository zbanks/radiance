import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Text {
    color: "#ddd"
    elide: Text.ElideMiddle
    style: Text.Raised
    styleColor: "#333"
    bottomPadding: 10
    topPadding: -5
    Rectangle {
        color: "#666"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 2
        anchors.leftMargin: -5
        anchors.rightMargin: -5
        anchors.bottomMargin: 2
  }
}
