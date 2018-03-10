import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

Dialog {
    visible: true
    title: "Color Kinetics Strip"
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {
        var vn = registry.deserialize(context, JSON.stringify({
            type: "KineticsStripOutputNode",
            host: hostField.text,
            width: widthField.text,
            height: heightField.text,
            dmxPort: dmxField.text,
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
            text: "Width"
        }
        TextField {
            id: widthField
            Layout.fillWidth: true

            Component.onCompleted: {
                widthField.forceActiveFocus();
            }
        }

        Label {
            text: "Height"
        }
        TextField {
            id: heightField
            Layout.fillWidth: true

            Component.onCompleted: {
                heightField.forceActiveFocus();
            }
        }

        Label {
            text: "First DMX Port"
        }
        TextField {
            id: dmxField
            Layout.fillWidth: true

            Component.onCompleted: {
                heightField.forceActiveFocus();
            }
        }
    }
}
