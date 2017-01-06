import QtQuick 2.3
import radiance 1.0

RadianceTile {
    property UIEffect uiEffect;
    readonly property Effect effect: (uiEffect == null) ? null : uiEffect.effect;
    implicitWidth: 200;
    implicitHeight: 300;
    borderWidth: 0;
}
