import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "."

Button {
    id: control
    property color colorDark: RadianceStyle.tileBackgroundColor
    property color colorLight: Qt.lighter(colorDark, 1.75)
    font.pixelSize: 16

    contentItem: Text {
        text: control.text
        font: control.font
        color: RadianceStyle.tileTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 60
        implicitHeight: 20

        border.width: 1
        border.color: control.activeFocus ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor
        Gradient {
            id: gradientPressed
            GradientStop { position: 0.5 ; color: control.colorDark }
            GradientStop { position: 1 ; color: control.colorLight }
        }
        Gradient {
            id: gradientReleased
            GradientStop { position: 0 ; color: control.colorLight }
            GradientStop { position: 0.5 ; color: control.colorDark }
        }
        gradient: control.pressed ? gradientPressed : gradientReleased
    }
}
