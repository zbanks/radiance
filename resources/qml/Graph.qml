import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

Item {
    property alias model: view.model
    property alias view: view
    property alias currentOutputName: viewWrapper.currentOutputName
    property alias lastClickedTile: viewWrapper.lastClickedTile
    Layout.fillWidth: true;
    Layout.fillHeight: true;

    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: view.width + 600;
        contentHeight: view.height + 400;
        clip: true;

        Item {
            id: viewWrapper
            property var lastClickedTile
            property string currentOutputName: ""
            width: Math.max(view.width + 400, flickable.width)
            height: Math.max(view.height + 400, flickable.height)

            View {
                id: view
                model: model
                delegates: {
                    "EffectNode": "EffectNodeTile",
                    "ImageNode": "ImageNodeTile",
                    "MovieNode": "MovieNodeTile",
                    "": "VideoNodeTile"
                }
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2

                Behavior on x { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }
                Behavior on y { PropertyAnimation { easing.type: Easing.InOutQuad; duration: 500; } }

                /*
                Rectangle {
                    opacity: 0.5
                    color: "red"
                    anchors.fill: parent
                }
                //*/
            }
        }
    }
}
