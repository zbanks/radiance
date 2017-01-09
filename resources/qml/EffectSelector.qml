import QtQuick 2.3
import radiance 1.0
import QtQuick.Controls 1.4

TextField {
    id: loadfield;
    visible: false;
    signal selected(string name);

    Keys.onPressed: {
        if (event.key == Qt.Key_Escape) {
            popdown();
        }
    }

    function popup() {
        visible = true;
        text = "";
        focus = true;
    }

    function popdown() {
        console.log("popdown");
        visible = false;
        focus = false;
    }

    onAccepted: {
        selected(text);
        popdown();
    }
}
