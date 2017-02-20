import QtQuick 2.3
import radiance 1.0
import QtQuick.Controls 1.4

ComboBox {
    id: loadfield;
    visible: false;
    model: EffectList.effectNames();
    editable: true;
    implicitWidth: 200;

    signal selected(string name);
    signal closed();

    Keys.onPressed: {
        if (event.key == Qt.Key_Escape) {
            popdown();
        } else if (event.key == Qt.Key_Enter) {
            onAccepted();
        }
    }

    onActiveFocusChanged: {
        if(!activeFocus) {
            popdown();
        }
    }

    function popup() {
        visible = true;
        focus = true;
        editText = "";
    }

    function popdown() {
        if(visible) {
            visible = false;
            focus = false;
            closed();
        }
    }

    onAccepted: {
        selected(currentText);
        popdown();
    }
}
