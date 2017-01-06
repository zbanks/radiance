import QtQuick 2.3
import radiance 1.0

RadianceTile {
    property UIEffect uiEffect;
    implicitWidth: 200;
    implicitHeight: 300;
    borderWidth: 0;

    function place() {
        uiEffect.x = 0;
        uiEffect.y = 0;
    }

    function load(name) {
        var component = Qt.createComponent("UIEffect.qml")
        var e = component.createObject(this);
        e.effect.source = name;
        var prev = uiEffect;
        uiEffect = e;
        if(prev != null) e.destroy();
        place();
    }

    function unload() {
        e.effect.source = name;
        if(uiEffect != null) {
            var prev = uiEffect;
            uiEffect = null;
            prev.destroy();
        }
    }
}
