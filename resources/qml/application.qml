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
        onGraphChanged: {
            console.log("Graph Changed");
        }
    }

    EffectNode {
        id: en
        name: "test"
    }
    EffectNode {
        id: en2
        name: "test"
    }

    Component.onCompleted: {
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "1024x768";
        model.addVideoNode(en);
        model.addVideoNode(en2);
        model.addEdge(en, en2, 0);
        RenderContext.addRenderTrigger(window, model, 0);
        console.log(model.graph.vertices[0]), 
        console.log(model.graph.vertices[1]), 
        console.log(model.graph.edges[0].fromVertex, 
                    model.graph.edges[0].toVertex, 
                    model.graph.edges[0].toInput);
        //en.tempPaint();
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }
}
