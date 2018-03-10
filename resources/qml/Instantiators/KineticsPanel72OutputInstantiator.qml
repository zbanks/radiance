import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

Dialog {
    visible: true
    title: "Color Kinetics Panel72"
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {
        var vn = registry.deserialize(context, JSON.stringify({
            type: "KineticsPanel72OutputNode",
            host: hostField.text,
            fullWidth: widthField.text,
            fullHeight: heightField.text,
            sectionStart: startField.text,
            sectionEnd: endField.text,
        }));
        if (vn) {
            graph.insertVideoNode(vn);
        } else {
            console.log("Could not instantiate MovieNode");
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: "PowerSupply IP"
        }
        TextField {
            id: hostField
            Layout.fillWidth: true

            Component.onCompleted: {
                hostField.forceActiveFocus();
            }
        }

        Label {
            text: "Full Display Panel Width"
        }
        TextField {
            id: widthField
            Layout.fillWidth: true

            Component.onCompleted: {
                widthField.forceActiveFocus();
            }
        }

        Label {
            text: "Full Display Panel Height"
        }
        TextField {
            id: heightField
            Layout.fillWidth: true

            Component.onCompleted: {
                heightField.forceActiveFocus();
            }
        }

        Label {
            text: "First Panel Position (starts at 1)"
        }
        TextField {
            id: startField
            Layout.fillWidth: true

            Component.onCompleted: {
                widthField.forceActiveFocus();
            }
        }

        Label {
            text: "Last Panel Position"
        }
        TextField {
            id: endField
            Layout.fillWidth: true

            Component.onCompleted: {
                heightField.forceActiveFocus();
            }
        }
    }
}
