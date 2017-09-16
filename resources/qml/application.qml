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
        onGraphChanged: {
            var changeset = "+" + verticesAdded.length + " -" + verticesRemoved.length + " vertices, ";
            changeset += "+" + edgesAdded.length + " -" + edgesRemoved.length + " edges";
            console.log("Graph changed!", changeset);
        }
    }

    // Make some nodes here to show it can be done; alternatively call model.createVideoNode(...)
    EffectNode {
        id: en
        name: "test"
    }
    EffectNode {
        id: en2
        name: "hue"
    }
    EffectNode {
        id: en3
        name: "tunnel"
    }
    EffectNode {
        id: en4
        name: "yellow"
    }
    ImageNode {
        id: img1
        imagePath: "nyancat.gif"
        inputCount: 1 // FIXME: this should be 0
    }
    EffectNode {
        id: cross
        name: "greenscreen"
        inputCount: 2
    }

    Component.onCompleted: {
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "1024x768";
        model.addVideoNode(en);
        model.addVideoNode(en2);
        model.addVideoNode(en3);
        model.addVideoNode(en4);
        model.addVideoNode(img1);
        model.addVideoNode(cross);

        model.addEdge(img1, en, 0);
        model.addEdge(en, en2, 0);
        model.addEdge(en2, en3, 0);
        model.addEdge(en3, en4, 0);
        model.addEdge(en4, cross, 0);

        var n1 = model.createVideoNode("test");
        var n2 = model.createVideoNode("interstellar");
        var n3 = model.createVideoNode("nogreen");
        model.addEdge(n1, n2, 0);
        model.addEdge(n2, n3, 0);
        model.addEdge(n3, cross, 1);

        model.flush();
        RenderContext.addRenderTrigger(window, model, 0);
    }

    RowLayout {
        anchors.fill: parent;

        ColumnLayout {

            RowLayout {
                Layout.fillWidth: true;

                // This is kind of crappy, but it was easy 
                ComboBox {
                    id: effectSelector;
                    model: Object.keys(NodeRegistry.nodeTypes);
                    editable: true;
                    Layout.preferredWidth: 300;
                }
                Button {
                    text: "Add"
                    onClicked: {
                        var node = model.createVideoNode(effectSelector.currentText);
                        model.flush();
                        console.log("New Node", effectSelector.currentText, node);
                    }
                }
            }

            Graph {
                model: model
            }
        }

        ColumnLayout {
            Waveform {
                width: 500
            }
            Spectrum {
                width: 500
            }
            Rectangle {
                color: "#000"
                width: 500
                height: 500
                VideoNodeRender {
                    id: vnr
                    anchors.fill: parent
                    chain: 0
                    videoNodeId: cross.id
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        vnr.update()
                        model.removeVideoNode(en4);
                        model.flush();
                    }
                }
            }
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }
}
