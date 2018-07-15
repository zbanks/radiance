import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
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
            GradientStop { position: 0; color: Qt.lighter(RadianceStyle.sliderKnobColor, 1.75) }
            GradientStop { position: 0.5; color: RadianceStyle.sliderKnobColor }
        }
        border.width: 1
        border.color: Qt.darker(RadianceStyle.sliderKnobColor, 1.5)
    }

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2

        implicitHeight: 6
        height: implicitHeight
        width: control.availableWidth
        radius: height/2
        border.width: 1
        border.color: Qt.darker(RadianceStyle.sliderTrackColor, 1.5)
        gradient: Gradient {
            GradientStop { position: 0.5; color: RadianceStyle.sliderTrackColor }
            GradientStop { position: 1; color: Qt.lighter(RadianceStyle.sliderTrackColor, 3) }
        }
        Rectangle {
            x: 1.5
            y: 1.5
            height: parent.height - 3
            width: control.visualPosition * (parent.width - 3)
            radius: height/2
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.lighter(RadianceStyle.sliderFillColor, 1.5) }
                GradientStop { position: 0.5; color: RadianceStyle.sliderFillColor }
            }
        }
    }
}
