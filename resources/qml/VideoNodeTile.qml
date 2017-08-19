import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

FocusScope {
    id: tile;
    property var videoNode;
    property var inputHeights;
    property int gridX;
    property int gridY;

    function sum(l) {
        var result = 0;
        for(var i=0; i<l.length; i++) result += l[i];
        return result;
    }

    function drop() {
        x = parent.width - (gridX + 1) * 100;
        y = gridY * 170;
    }

    onGridXChanged: {
        drop();
    }

    onGridYChanged: {
        drop();
    }

    width: 100;
    height: 170 * sum(inputHeights);

    Drag.active: dragArea.drag.active;
    Drag.keys: [ "videonode" ]

    states: State {
        when: dragArea.drag.active
        AnchorChanges { target: tile; anchors.verticalCenter: undefined; anchors.horizontalCenter: undefined }
    }

    MouseArea {
        id: dragArea;
        z: -1;
        anchors.fill: parent;
        onClicked: {
            tile.forceActiveFocus();
        }
        onReleased: {
            drop();
        }

        drag.onActiveChanged: {
            if(drag.active) {
                tile.z = 1;
            } else {
                tile.z = 0;
            }
        }

        drag.target: tile;
    }

    RadianceTile {
        anchors.fill: parent;
        focus: true;
    }

    Behavior on x {
        NumberAnimation {
            easing {
                type: Easing.OutElastic
                amplitude: 1.0
                period: 0.5
            }
        }
    }
    Behavior on y {
        NumberAnimation {
            easing {
                type: Easing.OutElastic
                amplitude: 1.0
                period: 0.5
            }
        }
    }
}
