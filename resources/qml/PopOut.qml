import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "."

Item {
    id: container
    property string side
    default property alias content: inner.children
    property real openSize: 250 // TODO rename to openSize
    property real extra: 10
    property real padding: 10
    property bool open: false
    property alias active: mouseArea.enabled
    width: side == "left" || side == "right" ? extra + (open ? openSize : 0) : undefined
    height: side == "top" || side == "bottom" ? extra + (open ? openSize : 0) : undefined

    function toggle() {
        open = !open;
    }

    anchors.right: side == "left" ? undefined : parent.right
    anchors.left: side == "right" ? undefined : parent.left
    anchors.top: side == "bottom" ? undefined : parent.top
    anchors.bottom: side == "top" ? undefined : parent.bottom
    clip: true

    Behavior on width {
        PropertyAnimation {
            easing.type: Easing.InOutQuad;
            duration: 300;
        }
    }
    Behavior on height {
        PropertyAnimation {
            easing.type: Easing.InOutQuad;
            duration: 300;
        }
    }

    Item {
        id: inner
        anchors.left: side == "left" ? undefined : parent.left
        anchors.right: side == "right" ? undefined : parent.right
        anchors.top: side == "top" ? undefined : parent.top
        anchors.bottom: side == "bottom" ? undefined : parent.bottom
        anchors.leftMargin: padding + (side == "right" ? extra : 0)
        anchors.rightMargin: padding + (side == "left" ? extra : 0)
        anchors.topMargin: padding + (side == "bottom" ? extra : 0)
        anchors.bottomMargin: padding + (side == "top" ? extra : 0)
        width: side == "left" || side == "right" ? openSize - 2 * padding : undefined
        height: side == "top" || side == "bottom" ? openSize - 2 * padding : undefined
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        opacity: 0.9
        color: RadianceStyle.mainBackgroundColor
    }

    Canvas {
        id: handle
        anchors.right: side == "right" ? inner.left : side == "left" ? undefined : parent.right
        anchors.left: side == "left" ? inner.right : side == "right" ? undefined : parent.left
        anchors.top: side == "top" ? inner.bottom : side == "bottom" ? undefined : parent.top
        anchors.bottom: side == "bottom" ? inner.top : side == "top" ? undefined : parent.bottom
        anchors.rightMargin: side == "right" ? padding - width / 2 : 0
        anchors.leftMargin: side == "left" ? padding - width / 2 : 0
        anchors.topMargin: side == "top" ? padding - height / 2 : 0
        anchors.bottomMargin: side == "bottom" ? padding - height / 2 : 0
        width: side == "left" || side == "right" ? 20 : undefined
        height: side == "top" || side == "bottom" ? 20 : undefined
        layer.enabled: true;

        property real lineWidth: 1;
        property real arrowWidth: 7;
        property real arrowHeight: 20;
        property color lineColor: RadianceStyle.mainLineColor;
        property real arrowDirection: open ? -1 : 1;

        onPaint: {
            var inset = lineWidth;

            var ctx = getContext("2d");
            ctx.reset();

            var sign = side == "right" || side == "bottom" ? -1 : 1;

            ctx.beginPath()
            if (side == "left" || side == "right") {
                ctx.moveTo(width / 2 + sign * inset, inset);
                ctx.lineTo(width / 2 + sign * inset, height / 2 - arrowHeight / 2);
                ctx.lineTo(width / 2 + sign * (inset + arrowWidth * arrowDirection), height / 2);
                ctx.lineTo(width / 2 + sign * inset, height / 2 + arrowHeight / 2);
                ctx.lineTo(width / 2 + sign * inset, parent.height - inset);
            } else {
                ctx.moveTo(inset, height / 2 + sign * inset);
                ctx.lineTo(width / 2 - arrowHeight / 2, height / 2 + sign * inset);
                ctx.lineTo(width / 2, height / 2 + sign * (inset + arrowWidth * arrowDirection));
                ctx.lineTo(width / 2 + arrowHeight / 2, height / 2 + sign * inset);
                ctx.lineTo(parent.width - inset, height / 2 + sign * inset);
            }
            ctx.lineWidth = lineWidth;
            ctx.strokeStyle = lineColor;
            ctx.stroke();
        }

        onArrowDirectionChanged: requestPaint();

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            anchors.margins: -5
            onClicked: toggle()
        }

        Behavior on arrowDirection {
            PropertyAnimation {
                easing.type: Easing.InOutQuad;
                duration: 300;
            }
        }
    }
}
