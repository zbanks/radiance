import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

ApplicationWindow {
    id: window;
    visible: true;

    Model {
        id: model;
        onVideoNodeAdded: {
            console.log("Added video node", videoNode);
        }
        onVideoNodeRemoved: {
            console.log("Removed video node", videoNode);
        }
        onEdgeAdded: {
            console.log("Added edge");
        }
        onEdgeRemoved: {
            console.log("Removed edge");
        }
    }

    EffectNode {
        id: en
        name: "yellow"
    }
    EffectNode {
        id: en2
        name: "heart"
    }
    EffectNode {
        id: en3
        name: "wwave"
    }
    EffectNode {
        id: en4
        name: "wwave"
    }

    Component.onCompleted: {
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "1024x768";
        model.addVideoNode(en);
        model.addVideoNode(en2);
        model.addVideoNode(en3);
        model.addVideoNode(en4);
        model.addEdge(en, en2, 0);
        model.addEdge(en2, en3, 0);
        //model.addEdge(en3, en4, 0);
        RenderContext.addRenderTrigger(window, model, 0);
    }

    ColumnLayout {
        Rectangle {
            color: "#FF0000"
            width: 500
            height: 500
            VideoNodeRender {
                id: vnr
                anchors.fill: parent
                chain: 0
                videoNode: en2
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    vnr.update()
                    model.removeVideoNode(en4);
                }
            }
        }

        View {
            model: model;
            delegates: {
                "EffectNode": "EffectNodeTile",
                "": "VideoNodeTile"
            }
            width: 500
            height: 500
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }
}
