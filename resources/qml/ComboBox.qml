import QtQuick 2.6
import QtQuick.Controls 2.1
import "."

ComboBox {
    id: control

    property color colorDark: RadianceStyle.tileBackgroundColor
    property color colorLight: Qt.lighter(colorDark, 1.75)
    property real modelWidth
    property real extraWidth: 18
    implicitWidth: modelWidth + leftPadding + rightPadding + extraWidth

    font.pixelSize: 14

    rightPadding: 3

    delegate: ItemDelegate {
        width: control.width
        height: control.height

        contentItem: Text {
            anchors.fill: parent
            anchors.margins: 2
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
            color: RadianceStyle.tileTextColor
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: highlighted ? RadianceStyle.tileBackgroundHighlightColor : "transparent"
        }
        highlighted: control.highlightedIndex === index
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"
        property real borderWidth: 1

        Connections {
            target: control
            onPressedChanged: canvas.requestPaint()
        }

        onPaint: {
            context.reset();
            context.moveTo(borderWidth * 2, borderWidth * 2);
            context.lineTo(width / 2, height - borderWidth * 2);
            context.lineTo(width - borderWidth * 2, borderWidth * 2);
            context.lineWidth = borderWidth;
            context.strokeStyle = RadianceStyle.tileLineColor;
            context.stroke();
        }
    }

    contentItem: Text {
        leftPadding: 3

        text: control.displayText
        font: control.font
        color: RadianceStyle.tileTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 60
        implicitHeight: 16
        color: RadianceStyle.tileBackgroundColor
        border.color: control.visualFocus ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor
        border.width: 1

        gradient: Gradient {
            GradientStop { position: 0 ; color: control.colorLight }
            GradientStop { position: 0.5 ; color: control.colorDark }
        }
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0 ; color: control.colorLight }
                GradientStop { position: 0.5 ; color: control.colorDark }
            }
            border.color: control.visualFocus ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor
        }
    }

    TextMetrics {
        id: textMetrics
        font: control.font
    }

    onModelChanged: {
        modelWidth = 0
        if (Array.isArray(model)) {
            for(var i = 0; i < model.length; i++) {
                textMetrics.text = model[i]
                modelWidth = Math.max(textMetrics.width, modelWidth)
            }
        } else {
            // TODO figure out how to iterate over models!
            modelWidth = 150
        }
    }
}
