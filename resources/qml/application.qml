import QtQuick 2.7
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import radiance 1.0
import "."

ApplicationWindow {
    id: window;
    visible: true
    color: "#333"
    width: 1200
    height: 800
    title: "Radiance"
    property bool hasMidi: false

    Context {
        id: globalContext;
        model: model;
        previewWindow: window;
    }

    Shortcut {
        id: saveAction
        sequence: StandaradKey.Save
        onActivated : {
            model.saveFile("radiance_state.json");
        }
    }
    Shortcut {
        id: loadAction
        sequence: StandardKey.Open
        onActivated: {
            model.loadFile("radiance_state.json");
            model.flush();
        }
    }
    Shortcut {
        sequence: StandardKey.Quit
        onActivated: Qt.quit();
    }
    header: ToolBar {
        leftPadding: 8
        Flow {
//            Layout.fillWidth: true;
            // This is kind of crappy, but it was easy
            LibraryWidget {
                id: library
                model: model
                graph: graph
            }
            ScreenWidget {
                id: screenWidget
                outputWindow: outputWindow
            }
            Loader {
                source: window.hasMidi ? "MidiMappingSelector.qml" : ""
                onLoaded: item.target = graph.view;
            }
            RowLayout {
                id: outputRow
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
                ToolButton {
                    text: "Stop saving to disk"
                    onClicked: {
                        outputImageSequence.stop();
                    }
                }
            }
            RowLayout {
                id: loadSaveRow
                ToolButton {
                    text: "Save"
                    onClicked: model.saveFile("radiance_state.json");
                }
                ToolButton {
                    text: "Load"
                    onClicked: {
                        model.loadFile("radiance_state.json");
                        model.flush();
                    }
                }
                ToolButton {
                    text: "Clear"
                    onClicked: {
                        model.clear();
                        model.flush();
                    }
                }
            }
        }
    }
    Model {
        id: model;
        onGraphChanged: {
            var changeset = "+" + verticesAdded.length + " -" + verticesRemoved.length + " vertices, ";
            changeset += "+" + edgesAdded.length + " -" + edgesRemoved.length + " edges, ";
            changeset += "+" + Object.keys(outputsAdded).length + " -" + Object.keys(outputsRemoved).length + " outputs";
            console.log("Graph changed!", changeset);
        }

        onMessage: {
            messages.text += "<font color=\"green\"><pre>" + str + "</pre></font>";
        }
        onWarning: {
            messages.text += "<font color=\"gold\"><pre>" + str + "</pre></font>";
        }
        onFatal: {
            messages.text += "<font color=\"red\"><pre>" + str + "</pre></font>";
        }
    }


    Timer {
        repeat: true
        running: true
        interval: 10 * 1000
        onTriggered: model.saveFile("radiance_state.json");
    }

    onClosing: {
        model.saveFile("radiance_state.json");
    }

    OutputWindow {
        id: outputWindow
        outputWindow.visible: found && screenWidget.outputVisibleChecked
        outputWindow.color: "black"

        OutputItem {
            id: outputItem
            parent: outputWindow.outputWindow.contentItem
            size: "1024x768"
            anchors.fill: parent

            Component.onCompleted: {
                output.name = "Screen"
            }

            Shortcut {
                sequence: StandardKey.Cancel
                onActivated : function() {
                    console.log("try to hide");
                    screenWidget.outputVisibleChecked = false;
                }
            }
        }

    }

    OutputImageSequence {
        id: outputImageSequence
        name: "ImageSequence"
        size: "500x500"
        fps: 60;
        enabled: outputImageSequenceCheckbox.checked;
    }

    Component.onCompleted: {
        Globals.context = globalContext;
        globalContext.outputs = [outputItem.output, outputImageSequence];
        model.loadFile("radiance_state.json");
        model.flush();

        if (model.vertices.length == 0) {
            // If the state was empty, then open up a few nodes as a demo
            var n1 = model.createVideoNode("nyancat.gif");
            var n2 = model.createVideoNode("test:0.4");
            var n3 = model.createVideoNode("interstellar:0.1");
            var cross = model.createVideoNode("crossfader");
            model.addEdge(n1, n2, 0);
            model.addEdge(n2, n3, 0);
            model.addEdge(n3, cross, 0);
            model.flush();
        }
    }

    ColumnLayout {
        anchors.fill: parent;

        RowLayout {
            Layout.fillWidth: true
            BeatIndicator {
                width: 25
                opacity: .9
            }
            Waveform {
                Layout.minimumWidth: 50
                Layout.fillWidth: true;
                opacity: .9
            }
            Spectrum {
                Layout.minimumWidth: 50
                Layout.fillWidth: true;
                opacity: .9
            }
        }

        Item {
            Layout.fillWidth: true;
            Layout.fillHeight: true;

            Graph {
                id: graph
                model: model
                currentOutputName: outputSelector.currentText
                anchors.fill: parent
            }


            Label {
                id: messages
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 10
                color: "white"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        messages.text = "";
                    }
                }
            }
        }
    }

}
