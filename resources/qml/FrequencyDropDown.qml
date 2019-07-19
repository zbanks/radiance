import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "."

ComboBox {
    id: control
    implicitHeight: 18
    implicitWidth: 45

    property real value: 0

    model: ListModel {
        id: model
        ListElement { text: "0"; value: 0 }
        ListElement { text: "1/8"; value: 0.125 }
        ListElement { text: "1/4"; value: 0.25 }
        ListElement { text: "1/2"; value: 0.5 }
        ListElement { text: "1"; value: 1 }
        ListElement { text: "2"; value: 2 }
        ListElement { text: "4"; value: 4 }
        ListElement { text: "8"; value: 8 }
    }

    textRole: "text"

    onCurrentIndexChanged: {
        value = model.get(currentIndex).value;
    }

    onValueChanged: {
        for (var i=0; i<model.count; i++) {
            if(model.get(i).value == value) {
                currentIndex = i;
                return;
            }
        }
        currentIndex = 0;
    }
}
