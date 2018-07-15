import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

ComboBox {
    id: control
    implicitHeight: 12
    implicitWidth: 20

    property real value: 0

    model: ListModel {
        id: model
        ListElement { img: "rest.svg"; value: 0 }
        ListElement { img: "whole.svg"; value: 0.25 }
        ListElement { img: "half.svg"; value: 0.5 }
        ListElement { img: "quarter.svg"; value: 1 }
        ListElement { img: "eighth.svg"; value: 2 }
        ListElement { img: "sixteenth.svg"; value: 4 }
        ListElement { img: "thirtysecond.svg"; value: 8 }
    }
    delegate: ItemDelegate {
        width: control.width
        height: 20
        Image {
            anchors.fill: parent
            source: "../graphics/" + img
            sourceSize.height: 128
            //height: 12
            //width: 20
            fillMode: Image.PreserveAspectFit
        }
    }
    indicator: Item {
    }
    contentItem: Item {
        id: contentItem
        property real oversample: 4
        scale: 1. / oversample
        Image {
            id: image
            anchors.centerIn: parent
            source: "../graphics/" + control.model.get(control.currentIndex).img
            fillMode: Image.PreserveAspectFit
            visible: false
            width: parent.width * contentItem.oversample
            height: parent.height * contentItem.oversample
            sourceSize.height: 128
        }
        ColorOverlay {
            anchors.fill: image
            source: image
            color: Style.tileTextColor
        }
    }
    background: Item {
    }

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
