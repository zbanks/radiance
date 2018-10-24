import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.1
import radiance 1.0
import "."

ApplicationWindow {
    title: "GLSL Editor"
    property VideoNode videoNode;
    visible: true;

    menuBar: MenuBar {
        //style: MenuBarStyle {
        //     background: Rectangle {
        //        implicitWidth: parent.width
        //        implicitHeight: parent.height
        //        color: RadianceStyle.editorBackgroundColor
        //        Rectangle {
        //            height: 1
        //            anchors.left: parent.left
        //            anchors.right: parent.right
        //            anchors.top: parent.bottom
        //            color: RadianceStyle.editorLineColor
        //        }
        //    }
        //}
        Menu {
            title: "&File"
            Action { text: "&New..." }
            Action { text: "&Open..." }
            Action { text: "&Save" }
            Action { text: "Save &As..." }
            MenuSeparator { }
            Action { text: qsTr("&Close") }
        }
    }

    TextArea {
        id: textArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        textFormat: Qt.RichText
        text: "Hello there!"
        Component.onCompleted: forceActiveFocus()
        font.family: "Monospace"
        style: TextAreaStyle {
            textColor: RadianceStyle.editorTextColor
            backgroundColor: RadianceStyle.editorBackgroundColor
        }
        frameVisible: false
    }
}
