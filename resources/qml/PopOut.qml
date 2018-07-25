import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "."

Item {
    id: container
    property string side
    default property alias content: inner.children
    property real openWidth: 250
    property real extra: 10
    property real padding: 10
    property bool open: false
    width: extra + (open ? openWidth : 0)

    function toggle() {
        open = !open;
    }

    anchors.right: side == "right" ? parent.right : undefined
    anchors.left: side == "left" ? parent.left : undefined
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    clip: true

    Behavior on width {
        PropertyAnimation {
            easing.type: Easing.InOutQuad;
            duration: 300;
        }
    }

    Item {
        id: inner
        anchors.left: side == "right" ? parent.left : undefined
        anchors.right: side == "left" ? parent.right : undefined
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: padding + (side == "right" ? extra : 0)
        anchors.rightMargin: padding + (side == "left" ? extra : 0)
        anchors.topMargin: padding
        anchors.bottomMargin: padding
        width: openWidth - 2 * padding
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        opacity: 0.9
        color: RadianceStyle.mainBackgroundColor
    }

    Canvas {
        id: handle
        anchors.right: side == "right" ? inner.left : undefined
        anchors.left: side == "left" ? inner.right : undefined
        anchors.rightMargin: side == "right" ? padding - width / 2 : 0
        anchors.leftMargin: side == "left" ? padding - width / 2 : 0
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 20
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

            var sign = side == "right" ? -1 : 1;

            ctx.beginPath()
            ctx.moveTo(width / 2 + sign * inset, inset);
            ctx.lineTo(width / 2 + sign * inset, height / 2 - arrowHeight / 2);
            ctx.lineTo(width / 2 + sign * (inset + arrowWidth * arrowDirection), height / 2);
            ctx.lineTo(width / 2 + sign * inset, height / 2 + arrowHeight / 2);
            ctx.lineTo(width / 2 + sign * inset, parent.height - inset);
            //ctx.lineTo((parent.width - arrowWidth - inset), (parent.height - inset));
            //ctx.lineTo((parent.width - arrowWidth - inset), (outputArrow + arrowHeight / 2));
            //ctx.lineTo((parent.width - inset) , outputArrow);
            //ctx.lineTo((parent.width - arrowWidth - inset), (outputArrow - arrowHeight / 2));
            //ctx.lineTo((parent.width - arrowWidth - inset), inset);
            ctx.lineWidth = lineWidth;
            ctx.strokeStyle = lineColor;
            ctx.stroke();
        }

        onArrowDirectionChanged: requestPaint();

        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
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
