import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0

Slider {
    id: control
    from: 0
    to: 1
    leftPadding: 0
    rightPadding: 0

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2

        height: 12
        width: 12
        radius: width/2
        gradient: Gradient {
            GradientStop { position: 0; color: "#777" }
            GradientStop { position: 0.5; color: "#444" }
        }
        border.width: 1
        border.color: "#222"
    }

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2

        implicitHeight: 6
        height: implicitHeight
        width: control.availableWidth
        radius: height/2
        border.width: 1
        border.color: "#000"
        gradient: Gradient {
            GradientStop { position: 0.5; color: "#000" }
            GradientStop { position: 1; color: "#333" }
        }
        Rectangle {
            x: 1.5
            y: 1.5
            height: parent.height - 3
            width: control.visualPosition * (parent.width - 3)
            radius: height/2
            gradient: Gradient {
                GradientStop { position: 0; color: "#66f" }
                GradientStop { position: 0.5; color: "#00f" }
            }
        }
    }
}
