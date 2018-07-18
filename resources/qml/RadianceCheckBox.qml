import QtQuick 2.6
import QtQuick.Controls 2.1
import "."

CheckBox {
    id: control
    spacing: 4
    padding: 1

    font.pointSize: 10

    property color colorDark: RadianceStyle.tileBackgroundColor
    property color colorLight: Qt.lighter(colorDark, 1.75)

    indicator: Rectangle {
        implicitHeight: 12
        implicitWidth: 12
        x: control.leftPadding
        y: parent.height / 2 - height / 2

        border.width: 1
        border.color: control.visualFocus ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor
        gradient: Gradient {
            GradientStop { position: 0 ; color: control.colorLight }
            GradientStop { position: 0.5 ; color: control.colorDark }
        }
        Rectangle {
            implicitHeight: parent.implicitHeight - 4
            implicitWidth: parent.implicitWidth - 4
            anchors.centerIn: parent
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.lighter(RadianceStyle.sliderFillColor, 1.5) }
                GradientStop { position: 0.5; color: RadianceStyle.sliderFillColor }
            }
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: RadianceStyle.tileTextColor
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
