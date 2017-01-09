import QtQuick 2.3
import radiance 1.0
import QtQuick.Controls 1.4

TextField {
    id: loadfield;
    visible: false;
    signal selected(string name);
    signal closed();

    Keys.onPressed: {
        if (event.key == Qt.Key_Escape) {
            popdown();
        }
    }

    onActiveFocusChanged: {
        if(!activeFocus) {
            popdown();
        }
    }

    function popup() {
        visible = true;
        text = "";
        focus = true;
    }

    function popdown() {
        if(visible) {
            visible = false;
            focus = false;
            closed();
        }
    }

    onAccepted: {
        selected(text);
        popdown();
    }
}
