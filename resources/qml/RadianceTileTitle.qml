import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."

Text {
    color: RadianceStyle.tileTextColor
    elide: Text.ElideMiddle
    horizontalAlignment: Text.AlignHCenter
    font.pointSize: 10
    Rectangle {
        color: RadianceStyle.tileLineColor
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        anchors.leftMargin: -3
        anchors.rightMargin: -3
        anchors.bottomMargin: -3
    }
}
