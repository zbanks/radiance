import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

Dialog {
    visible: true
    title: "MPV"
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    onAccepted: {
        var vn = registry.deserialize(context, JSON.stringify({
            type: "MovieNode",
            file: textbox.text,
            name: textbox.text
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
            text: "Instantiate MPV:"
        }
        TextField {
            id: textbox
            Layout.fillWidth: true

            Component.onCompleted: {
                textbox.forceActiveFocus();
            }
        }
    }
}
