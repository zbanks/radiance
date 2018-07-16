import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import "."

Button {
    id: control
    property color colorDark: RadianceStyle.tileBackgroundColor
    property color colorLight: Qt.lighter(colorDark, 1.75)

    style: ButtonStyle {
        id: style

        background: Rectangle {
            implicitWidth: 100
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
}
