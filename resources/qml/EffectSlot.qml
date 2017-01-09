import QtQuick 2.3
import radiance 1.0
import QtQuick.Controls 1.4

RadianceTile {
    id: tile;
    property UIEffect uiEffect;
    property EffectSelector effectSelector;

    implicitWidth: 200;
    implicitHeight: 300;
    borderWidth: 0;

    function place() {
    }

    function onChildKey(event) {
        if (event.key == Qt.Key_Delete) {
            unload();
        }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Colon) {
            effectSelector.x = x + 50;
            effectSelector.y = y + 50;
            effectSelector.selected.connect(load);
            effectSelector.popup();
        }
    }

    function load(name) {
        var component = Qt.createComponent("UIEffect.qml")
        var e = component.createObject(this);
        e.effect.source = name;
        e.Keys.onPressed.connect(onChildKey);

        var prev = uiEffect;
        uiEffect = e;
        if(prev != null) prev.destroy();
        place();
        effectSelector.popdown();
    }

    function unload() {
        if(uiEffect != null) {
            var prev = uiEffect;
            uiEffect = null;
            prev.destroy();
        }
    }

    onActiveFocusChanged: {
        if(!activeFocus) {
            //effectSelector.popdown();
        }
    }

    MouseArea {
        anchors.fill: parent;
        onClicked: {
            tile.forceActiveFocus();
        }
    }
}
