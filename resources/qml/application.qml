import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0
import "."

ApplicationWindow {
    id: window;
    visible: true;
    color: "#333";

    Context {
        id: globalContext;
        model: model;
        previewWindow: window;
    }

    Model {
        id: model;
        onGraphChanged: {
            var changeset = "+" + verticesAdded.length + " -" + verticesRemoved.length + " vertices, ";
            changeset += "+" + edgesAdded.length + " -" + edgesRemoved.length + " edges, ";
            changeset += "+" + Object.keys(outputsAdded).length + " -" + Object.keys(outputsRemoved).length + " outputs";
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

    OutputWindow {
        id: outputWindow
        window.visible: screenWidget.outputVisibleChecked
        screen: screenWidget.screenSelected
        window.color: "black"

        OutputItem {
            id: outputItem
            parent: outputWindow.window.contentItem
            size: "1024x768"
            anchors.fill: parent

            Component.onCompleted: {
                output.name = "Screen"
            }
        }
    }

    OutputImageSequence {
        id: outputImageSequence
        name: "ImageSequence"
        size: "500x500"
        fps: 30;
        enabled: outputImageSequenceCheckbox.checked;
    }

    Component.onCompleted: {
        Globals.context = globalContext;
        globalContext.outputs = [outputItem.output, outputImageSequence];
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "1024x768";
        model.addVideoNode(en);
        model.addVideoNode(en2);
        model.addVideoNode(en3);
        model.addVideoNode(en4);
        model.addVideoNode(img1);
        model.addVideoNode(cross);

        //model.addEdge(img1, en, 0);
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
    }

    ColumnLayout {
        anchors.fill: parent;

        RowLayout {
            Layout.fillWidth: true;

            // This is kind of crappy, but it was easy 
            ComboBox {
                id: nodeSelector;
                model: Object.keys(NodeRegistry.nodeTypes);
                editable: true;
                Layout.preferredWidth: 300;
                onAccepted: nodeAddAction.trigger()
            }
            Action {
                id: nodeAddAction
                onTriggered: {
                    var node = model.createVideoNode(nodeSelector.currentText);
                    model.flush();
                    console.log("New Node", nodeSelector.currentText, node);
                }
            }
            Button {
                id: nodeAddButton
                text: "Add"
                action: nodeAddAction
            }
            Button {
                id: nodeRegistryReload
                text: "Reload Registry"
                onClicked: {
                    NodeRegistry.reload();
                }
            }

            ScreenWidget {
                id: screenWidget
                outputWindow: outputWindow
            }

            ComboBox {
                id: outputSelector;
                function outputNames() {
                    var o = globalContext.outputs;
                    var a = [];
                    for (var i=0; i<o.length; i++) {
                        a.push(o[i].name);
                    }
                    return a;
                }
                model: outputNames()
            }

            CheckBox {
                id: outputImageSequenceCheckbox
                text: "Save to disk"
            }
            Button {
                text: "Stop saving to disk"
                onClicked: {
                    outputImageSequence.stop();
                }
            }
        }

        Item {
            Layout.fillWidth: true;
            Layout.fillHeight: true;

            Graph {
                model: model
                currentOutputName: outputSelector.currentText
                anchors.fill: parent
            }

            RowLayout {
                Waveform {
                    width: 500
                    opacity: .9
                }
                Spectrum {
                    width: 500
                    opacity: .9
                }
            }
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }

    Action {
        id: newNodeAction
        text: "&New Node"
        shortcut: ":"
        onTriggered: nodeSelector.forceActiveFocus()
    }
}
