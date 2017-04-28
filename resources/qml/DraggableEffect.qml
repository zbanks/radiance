import QtQuick 2.3
import QtQuick.Controls 1.4

UIEffect {
    id: tile;
    property var slot;

    anchors.horizontalCenter: parent.horizontalCenter;
    anchors.verticalCenter: parent.verticalCenter;
    Keys.onPressed: slot ? slot.onChildKey : null;

    Drag.active: dragArea.drag.active;
    Drag.keys: [ "effect" ]

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
        }

        drag.onActiveChanged: {
            if(drag.active) {
                tile.parent = tile.parent.parent.parent.parent.parent;
            } else {
                if(tile.Drag.target) {
                    tile.parent = tile.Drag.target.parent;
                    tile.parent.replace(tile);
                } else {
                    tile.parent = tile.slot;
                }
            }
        }

        drag.target: tile;
    }
}
