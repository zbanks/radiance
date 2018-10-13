import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import radiance 1.0
import "."

VideoNodeTile {
    id: tile;

    normalHeight: 260;
    normalWidth: 200;
    property bool updatingResolutionSelector: false;
    property bool updatingScreenSelector: false;

    function updateResolutionSelector() {
        var l = videoNode.suggestedResolutions;
        var strList = [];
        for (var i=0; i<l.length; i++) {
            strList.push(l[i].width + "x" + l[i].height);
        }
        var currentRes = videoNode.resolution.width + "x" + videoNode.resolution.height;
        if (strList.indexOf(currentRes) < 0) {
            strList.unshift(currentRes);
        }

        updatingResolutionSelector = true;
        resolutionSelector.model = strList;

        var i = strList.indexOf(currentRes);
        if (i >= 0) {
            resolutionSelector.currentIndex = i;
        }
        updatingResolutionSelector = false;
    }

    function updateScreenSelector() {
        var l = videoNode.availableScreens;
        var strList = [];
        for (var i=0; i<l.length; i++) {
            strList.push(l[i]);
        }
        var n = tile.videoNode.screenName;
        if (strList.indexOf(n) < 0) {
            strList.unshift(n);
        }
        updatingScreenSelector = true;
        screenSelector.model = strList;

        var i = strList.indexOf(n);
        if (i >= 0) {
            screenSelector.currentIndex = i;
        }
        updatingScreenSelector = false;
    }

    onVideoNodeChanged: {
        visibleCheck.checked = videoNode.shown;
        if (!videoNode.shown) {
            updateScreenSelector();
            updateResolutionSelector();
        }
        videoNode.shown = Qt.binding(function() { return visibleCheck.checked });
    }

    Connections {
        target: videoNode
        onShownChanged: {
            visibleCheck.checked = shown;
            if (!shown) {
                updateScreenSelector();
                updateResolutionSelector();
            }
        }
        onAvailableScreensChanged: {
            if (!videoNode.shown) {
                updateScreenSelector();
            }
        }
        onScreenNameChanged: {
            if (!videoNode.shown) {
                updateScreenSelector();
            }
        }
        onSuggestedResolutionsChanged: {
            if (!videoNode.shown) {
                updateResolutionSelector();
            }
        }
        onResolutionChanged: {
            if (!videoNode.shown) {
                updateResolutionSelector();
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent;
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 5
        anchors.topMargin: 5

        RadianceTileTitle {
            Layout.fillWidth: true
            text: "Screen Output"
        }

        CheckerboardPreview {
            videoNode: tile.videoNode
        }

        RowLayout {
            ComboBox {
                id: screenSelector;
                enabled: !visibleCheck.checked;
                onCurrentTextChanged: {
                    if (!updatingScreenSelector) {
                        videoNode.screenName = currentText;
                    }
                }
                Layout.fillWidth: true;
            }
            Text {
                text: "\u2717"
                visible: !videoNode.found;
                color: "red"
            }
        }

        ComboBox {
            id: resolutionSelector;
            enabled: !visibleCheck.checked;
            onCurrentTextChanged: {
                if (!updatingResolutionSelector) {
                    videoNode.resolution = currentText;
                }
            }
            Layout.fillWidth: true;
        }

        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: visibleCheck
                text: "Visible"
            }
        }
    }
}
