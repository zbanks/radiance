import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

Rectangle {
    property alias model: view.model
    Layout.fillWidth: true;
    Layout.fillHeight: true;
    border.width: 1
    color: "transparent"

    ScrollView {
        anchors.fill: parent;

        Flickable {
            property var lastClickedTile

            anchors.fill: parent
            contentWidth: view.width;
            contentHeight: view.height;
            clip: true;

            View {
                id: view
                model: model
                delegates: {
                    "EffectNode": "EffectNodeTile",
                    "ImageNode": "ImageNodeTile",
                    "": "VideoNodeTile"
                }
                /*
                Rectangle {
                    opacity: 0.5
                    color: "red"
                    anchors.fill: parent
                }
                */
            }
        }
    }
}
