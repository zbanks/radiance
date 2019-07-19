import QtQuick 2.6
import QtQuick.Controls 2.1
import "."

TextField {
    id: control
    font.pixelSize: 16

    color: RadianceStyle.tileTextColor
    padding: 3

    background: Rectangle {
        color: RadianceStyle.tileBackgroundColor
        border.color: control.visualFocus ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor
        border.width: 1

        gradient: Gradient {
            GradientStop { position: 0 ; color: control.colorLight }
            GradientStop { position: 0.5 ; color: control.colorDark }
        }
    }
}
