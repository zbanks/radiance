pragma Singleton
import QtQuick 2.7

QtObject {
    // Master color definitions
    readonly property color gray: "#333"
    readonly property color black: "#111"
    readonly property color white: "#aaa"
    readonly property color accent: "#60a"

    // These colors are for the main UI background
    readonly property color mainBackgroundColor: gray
    readonly property color mainTextColor: white
    readonly property color mainTextHighlightColor: Qt.lighter(mainTextColor, 1.5)
    readonly property color mainLineColor: white

    // These colors are for the tiles
    readonly property color tileBackgroundColor: black
    readonly property color tileBackgroundHighlightColor: Qt.darker(accent, 3)
    readonly property color tileTextColor: Qt.lighter(white, 1.5)
    readonly property color tileLineColor: Qt.darker(white, 1.5)
    readonly property color tileLineHighlightColor: Qt.lighter(white, 1.5)

    // Colors for UI elements
    readonly property color sliderKnobColor: gray
    readonly property color sliderTrackColor: black
    readonly property color sliderFillColor: accent

    // Indicators
    readonly property color ballColor: accent
    readonly property color spectrumColor: accent
    readonly property color waveformColor: accent
    readonly property color dropTargetColor: Qt.lighter(accent, 2)
}  
