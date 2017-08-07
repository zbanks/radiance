import QtQuick 2.3
import radiance 1.0
import QtQuick.Controls 1.4

RadianceTile {
    id: tile;
    property DraggableEffect uiEffect;
    property EffectSelector effectSelector;

    borderWidth: 0;

    function onChildKey(event) {
        if (event.key == Qt.Key_Delete) {
            unload();
        }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Colon) {
            effectSelector.x = x;
            effectSelector.y = parent.y + 10;
            effectSelector.selected.connect(selectorSelected);
            effectSelector.closed.connect(selectorClosed);
            effectSelector.popup();
        }
    }

    function selectorSelected(name) {
        selectorClosed();
        load(name);
    }

    function selectorClosed() {
        effectSelector.selected.disconnect(selectorSelected);
        effectSelector.closed.disconnect(selectorClosed);
    }

    function load(name) {
        var component = Qt.createComponent("DraggableEffect.qml")
        var e = component.createObject(tile);
        e.effect.name = name;
        radmodel.addVideoNode(e.effect);

        replace(e);
        effectSelector.popdown();
    }

    function replace(e) {
        if(e == uiEffect) return;
        var prev = uiEffect;
        uiEffect = e;
        if(prev) prev.destroy();
        if(e.slot) e.slot.uiEffect = null;
        e.slot = tile;
    }

    function unload() {
        if(uiEffect != null) {
            var prev = uiEffect;
            uiEffect = null;
            prev.destroy();
        }
    }

    MouseArea {
        anchors.fill: parent;
        onClicked: {
            tile.forceActiveFocus();
        }
    }

    DropArea {
        keys: ["effect"]
        anchors.fill: parent;
    }
}
