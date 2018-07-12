import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Canvas {
    property bool selected: false;
    property color startColor: selected ? "#116" : "#111";
    property color endColor: selected ? "#181868" : "#181818";
    property real borderWidth: 3;
    property color borderColor: activeFocus ? "#A92" : "#666";
    property int padding: 5;
    property int blockWidth: 100;
    property int blockHeight: 170;
    layer.enabled: true;

    property var inputArrows: []
    property real outputArrow: height / 2;
    property real arrowSize: 10;

    anchors.rightMargin: -arrowSize;

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();

        var grd = ctx.createLinearGradient(0, 0, 0, height);
        grd.addColorStop(0, startColor);
        grd.addColorStop(1, endColor);

        ctx.beginPath()
        ctx.moveTo(0, 0);
        for (var i=0; i<inputArrows.length; i++) {
            var y = inputArrows[i];
            ctx.lineTo(0, y - arrowSize);
            ctx.lineTo(arrowSize, y);
            ctx.lineTo(0, y + arrowSize);
        }
        ctx.lineTo(0, height);
        ctx.lineTo(width - arrowSize, height);
        ctx.lineTo(width - arrowSize, outputArrow + arrowSize);
        ctx.lineTo(width, outputArrow);
        ctx.lineTo(width - arrowSize, outputArrow - arrowSize);
        ctx.lineTo(width - arrowSize, 0);
        ctx.closePath();
        ctx.fillStyle = grd;
        ctx.fill();
        ctx.strokeStyle = "red";
        ctx.stroke();
    }

    onInputArrowsChanged: requestPaint();
    onOutputArrowChanged: requestPaint();
}
