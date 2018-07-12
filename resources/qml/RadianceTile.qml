import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Canvas {
    property bool selected: false;
    property color darkColor: selected ? "#113" : "#111";
    property color lightColor: selected ? "#224" : "#222";
    property real borderWidth: 2;
    property color borderColor: "#666";
    property int padding: 5;
    property int blockWidth: 100;
    property int blockHeight: 170;
    layer.enabled: true;

    property var inputArrows: []
    property real outputArrow: height / 2;
    property real arrowWidth: 10;
    property real arrowHeight: 30;

    anchors.rightMargin: -arrowWidth - borderWidth / 2;
    anchors.leftMargin: -borderWidth / 2;

    onPaint: {
        var inset = borderWidth / 2;

        var ctx = getContext("2d");
        ctx.reset();

        var fill = ctx.createLinearGradient(0, 0, 0, 100);
        fill.addColorStop(0, lightColor);
        fill.addColorStop(1, darkColor);

        ctx.beginPath()
        ctx.moveTo(inset, inset);
        for (var i=0; i<inputArrows.length; i++) {
            var y = inputArrows[i];
            ctx.lineTo(inset, y - arrowHeight / 2);
            ctx.lineTo(arrowWidth + inset, y);
            ctx.lineTo(inset, y + arrowHeight / 2);
        }
        ctx.lineTo(inset, height - inset);
        ctx.lineTo(width - arrowWidth - inset, height - inset);
        ctx.lineTo(width - arrowWidth - inset, outputArrow + arrowHeight / 2);
        ctx.lineTo(width - inset, outputArrow);
        ctx.lineTo(width - arrowWidth - inset, outputArrow - arrowHeight / 2);
        ctx.lineTo(width - arrowWidth - inset, inset);
        ctx.closePath();
        ctx.fillStyle = fill;
        ctx.fill();
        ctx.lineWidth = borderWidth;
        ctx.strokeStyle = borderColor;
        ctx.stroke();
    }

    onInputArrowsChanged: requestPaint();
    onOutputArrowChanged: requestPaint();
    onSelectedChanged: requestPaint();
}
