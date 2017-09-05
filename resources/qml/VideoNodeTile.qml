import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import radiance 1.0

FocusScope {
    id: tile;
    property var model;
    property var videoNode;
    property var inputHeights;
    property int gridX;
    property int gridY;

    property int padding: 5;
    property int blockWidth: 100;
    property int blockHeight: 170;
    property bool selected: false;

    property var lastX;
    property var lastY;
    property var dragObjects;
    property var dragging;

    function sum(l) {
        var result = 0;
        for(var i=0; i<l.length; i++) result += l[i];
        return result;
    }

    function regrid() {
        x = parent.width - (gridX + 1) * (blockWidth + padding);
        y = (gridY + 0.5 * (inputHeights[0] - 1)) * (blockHeight + padding);
        height = (blockHeight + padding) * (sum(inputHeights) - (inputHeights[inputHeights.length - 1] - 1)) - padding;
    }

    onGridXChanged: {
        if (!dragging) regrid();
    }

    onGridYChanged: {
        if (!dragging) regrid();
    }

    onInputHeightsChanged: {
        if (!dragging) regrid();
    }

    width: blockWidth;

    Drag.keys: [ "videonode" ]
    Drag.hotSpot: Qt.point(width / 2, height / 2)
    Drag.active: dragArea.drag.active;

    function dragLift() {
        var ccs = parent.selectedConnectedComponents();
        var i;
        for (i=0; i<ccs.length; i++) {
            if (ccs[i].tiles.indexOf(tile) >= 0) break;
        }
        if (i == ccs.length) return; // Drag object was not found in selection??
        dragObjects = ccs[i].tiles;
        lastX = x;
        lastY = y;
        for (var i=0; i<dragObjects.length; i++) {
            dragObjects[i].dragging = true;
            dragObjects[i].z = 1;
        }
    }

    function dragDrop() {
        for (var i=0; i<dragObjects.length; i++) {
            dragObjects[i].dragging = false;
            dragObjects[i].z = 0;
        }

        var t = Drag.target;
        var me = videoNode;
        if (t !== null
         && t.fromNode != me
         && t.toNode != me) {
            var fn = t.fromNode;
            var tn = t.toNode;
            var ti = t.toInput;
            var e = model.edges;
            var v = model.vertices;
            // We can only move the whole tree if it would not create a cycle and fromNode is empty
            // Maybe this should be controlled by a keyboard shortcut like Shift?
            var meOnly = fn !== null || model.isAncestor(tn, me);
            // Keep track of removed connections so we can splice them back together
            var prevFromVertex = null;
            var prevToVertex = null;
            var prevToInput = null;
            var toRemove = [];
            for (var i=0; i<e.length; i++) {
                if (v[e[i].fromVertex] == me) {
                    toRemove.push([v[e[i].fromVertex], v[e[i].toVertex], e[i].toInput]);
                    prevToVertex = v[e[i].toVertex];
                    prevToInput = e[i].toInput;
                }
                if (meOnly && v[e[i].toVertex] == me) {
                    toRemove.push([v[e[i].fromVertex], v[e[i].toVertex], e[i].toInput]);
                    prevFromVertex = v[e[i].fromVertex];
                }
            }
            for (var i=0; i<toRemove.length; i++) {
                console.log(toRemove[i]);
                model.removeEdge(toRemove[i][0],toRemove[i][1],toRemove[i][2]);
            }
            if (prevFromVertex !== null && prevToVertex !== null & prevToInput !== null) {
                console.log(prevFromVertex, prevToVertex, prevToInput);
                model.addEdge(prevFromVertex, prevToVertex, prevToInput);
            }
            if (fn !== null) {
                console.log(fn+" ...");
                model.addEdge(fn, me, 0);
                console.log(fn+" OK");
            }
            if (tn !== null) {
                model.addEdge(me, tn, ti);
            }
        }
        for (var i=0; i<dragObjects.length; i++) {
            dragObjects[i].regrid();
        }
    }

    MouseArea {
        id: dragArea;
        z: -1;
        anchors.fill: parent;

        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                tile.forceActiveFocus();
                if (mouse.modifiers & Qt.ShiftModifier) {
                    tile.parent.addToSelection([tile]);
                } else if (mouse.modifiers & Qt.ControlModifier) {
                    tile.parent.toggleSelection([tile]);
                } else if (mouse.modifiers & Qt.AltModifier) {
                    tile.parent.removeFromSelection([tile]);
                } else {
                    tile.parent.select([tile]);
                }
            }
        }

        drag.onActiveChanged: {
            tile.parent.ensureSelected(tile);
            if (drag.active) {
                dragLift();
            } else {
                dragDrop();
            }
        }

        drag.target: tile;
    }

    RadianceTile {
        anchors.fill: parent;
        selected: parent.selected;
        focus: true;
    }

    Behavior on x {
        enabled: !dragging
        NumberAnimation {
            easing {
                type: Easing.InOutQuad
                amplitude: 1.0
                period: 0.5
            }
            duration: 500
        }
    }
    Behavior on y {
        enabled: !dragging
        NumberAnimation {
            easing {
                type: Easing.InOutQuad
                amplitude: 1.0
                period: 0.5
            }
            duration: 500
        }
    }
    Behavior on height {
        enabled: !dragging
        NumberAnimation {
            easing {
                type: Easing.InOutQuad
                amplitude: 1.0
                period: 0.5
            }
            duration: 500
        }
    }

    onXChanged: {
        if (Drag.active) {
            var deltaX = x - lastX;
            for(var i = 0; i < dragObjects.length; ++i) {
                if (dragObjects[i] != this) {
                    dragObjects[i].x += deltaX;
                }
            }
            lastX = x;
        }
    }

    onYChanged: {
        if (Drag.active) {
            var deltaY = y - lastY;
            for(var i = 0; i < dragObjects.length; ++i) {
                if (dragObjects[i] != this) {
                    dragObjects[i].y += deltaY;
                }
            }
            lastY = y;
        }
    }
}
