import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RowLayout {
    property int count: 4;
    property int layout: Qt.LeftToRight;
    layoutDirection: layout;

    function first() {
        return repeater.itemAt(0).effect;
    }

    function last() {
        return repeater.itemAt(repeater.count - 1).effect;
    }

    Repeater {
        id: repeater;
        model: parent.count;

        ColumnLayout {
            property alias effect: uiEffect.effect;

            UIEffect {
                id: uiEffect;
                effect.previous: index == 0 ? null : repeater.itemAt(index - 1).effect;
                effect.source: effectName.currentText;
            }
               
            ComboBox {
                id: effectName;
                Layout.fillWidth: true;
                editable: true;
                currentIndex: index;
                model: ["purple", "test", "circle", "rainbow"];
            }
        }
    }
}
