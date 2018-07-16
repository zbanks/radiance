import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0
import "."

Item {
    property real oversample: 1
    property alias inputArrows: canvas.inputArrows
    property alias outputArrow: canvas.outputArrow
    property alias arrowWidth: canvas.arrowWidth
    property alias arrowHeight: canvas.arrowHeight
    property alias selected: canvas.selected;
    property alias highlight: canvas.highlight;
    property alias bottomColor: canvas.bottomColor;
    property alias topColor: canvas.topColor;
    property alias borderWidth: canvas.borderWidth;
    property alias borderColor: canvas.borderColor;
    property alias padding: canvas.padding;
    property alias blockWidth: canvas.blockWidth;
    property alias blockHeight: canvas.blockHeight;

    scale: 1 / oversample

    anchors.rightMargin: -arrowWidth - borderWidth / 2;
    anchors.leftMargin: -borderWidth / 2;

    Canvas {
        id: canvas
        property bool selected: false;
        property bool highlight: false;
        property color bottomColor: selected ? RadianceStyle.tileBackgroundHighlightColor : RadianceStyle.tileBackgroundColor;
        property color topColor: Qt.lighter(bottomColor, 1.75);
        property real borderWidth: 1;
        property color borderColor: highlight ? RadianceStyle.tileLineHighlightColor : RadianceStyle.tileLineColor;
        property int padding: 3;
        property int blockWidth: 65;
        property int blockHeight: 110;
        layer.enabled: true;

        property var inputArrows: []
        property real outputArrow: parent.height / 2;
        property real arrowWidth: 7;
        property real arrowHeight: 20;

        anchors.centerIn: parent
        width: parent.width * oversample
        height: parent.height * oversample

        onPaint: {
            var inset = borderWidth / 2;

            var ctx = getContext("2d");
            ctx.reset();

            var fill = ctx.createLinearGradient(0, 0, 0, 100 * oversample);
            fill.addColorStop(0, topColor);
            fill.addColorStop(1, bottomColor);

            ctx.beginPath()
            ctx.moveTo(inset * oversample, inset * oversample);
            for (var i=0; i<inputArrows.length; i++) {
                var y = inputArrows[i];
                ctx.lineTo(inset * oversample, (y - arrowHeight / 2) * oversample);
                ctx.lineTo((arrowWidth + inset) * oversample, y * oversample);
                ctx.lineTo(inset * oversample, (y + arrowHeight / 2) * oversample);
            }
            ctx.lineTo(inset * oversample, (parent.height - inset) * oversample);
            ctx.lineTo((parent.width - arrowWidth - inset) * oversample, (parent.height - inset) * oversample);
            ctx.lineTo((parent.width - arrowWidth - inset) * oversample, (outputArrow + arrowHeight / 2) * oversample);
            ctx.lineTo((parent.width - inset) * oversample, outputArrow * oversample);
            ctx.lineTo((parent.width - arrowWidth - inset) * oversample, (outputArrow - arrowHeight / 2) * oversample);
            ctx.lineTo((parent.width - arrowWidth - inset) * oversample, inset * oversample);
            ctx.closePath();
            ctx.fillStyle = fill;
            ctx.fill();
            ctx.lineWidth = borderWidth * oversample;
            ctx.strokeStyle = borderColor;
            ctx.stroke();
        }

        onInputArrowsChanged: requestPaint();
        onOutputArrowChanged: requestPaint();
        onSelectedChanged: requestPaint();
        onHighlightChanged: requestPaint();
    }
}
