import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

Dialog {
    visible: true
    title: "Youtube Search"
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {
        var vn = registry.deserialize(context, JSON.stringify({
            type: "LightOutputNode",
            url: textbox.text,
        }));
        if (vn) {
            graph.insertVideoNode(vn);
        } else {
            console.log("Could not instantiate LightOutputNode");
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: "Enter device URL:"
        }
        TextField {
            id: textbox
            Layout.fillWidth: true
            text: "localhost"

            Component.onCompleted: {
                textbox.forceActiveFocus();
            }
        }
    }
}
