import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0
import "."

ApplicationWindow {
    id: window;
    visible: true
    color: "#333"
    width: 800
    height: 530
    title: "Radiance"
    property bool hasMidi: false

    PreviewAdapter {
        id: previewAdapter;
        model: model;
        previewWindow: window;
    }

    Registry {
        id: registry;
    }

    Model {
        id: model;
        onGraphChanged: {
            var changeset = "+" + verticesAdded.length + " -" + verticesRemoved.length + " vertices, ";
            changeset += "+" + edgesAdded.length + " -" + edgesRemoved.length + " edges";
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

    Action {
        id: saveAction
        shortcut: "Ctrl+S"
        onTriggered: {
            if (model.vertices.length >= 0) {
                model.save(modelName.currentText);
            }
        }
    }

    Action {
        id: loadAction
        shortcut: "Ctrl+R"
        onTriggered: {
            console.log("Loading state from file...");
            model.load(defaultContext, registry, modelName.currentText);
            model.flush();
        }
    }

    Timer {
        repeat: true
        running: true
        interval: 10 * 1000
        onTriggered: saveAction.trigger()
    }

    /*
    // Make some nodes here to show it can be done; alternatively call model.createVideoNode(...)
    EffectNode {
        id: cross
        name: "crossfader"
        inputCount: 2
    }
    */

    Component.onCompleted: {
        Globals.previewAdapter = previewAdapter;

        loadAction.trigger();
        if (model.vertices.length == 0) {
            // If the state was empty, then open up a few nodes as a demo
            model.load(defaultContext, registry, "gui_default");
        }
    }

    ColumnLayout {
        anchors.fill: parent;

        RowLayout {
            Layout.fillWidth: true;

            Loader {
                source: window.hasMidi ? "MidiMappingSelector.qml" : ""
                onLoaded: {
                    item.target = graph.view;
                }
            }

            Button {
                text: "Save"
                action: saveAction
            }
            Button {
                text: "Load"
                action: loadAction
            }
            Button {
                text: "Clear"
                onClicked: {
                    model.clear();
                    model.flush();
                }
            }
            Label {
                text: "Model File:"
                color: "#eee"
            }
            ComboBox {
                id: modelName
                editable: true
                model: ["gui", "cli"]
            }
        }

        Item {
            Layout.fillWidth: true;
            Layout.fillHeight: true;
            Graph {
                id: graph
                model: model
                anchors.fill: parent
            }

            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    BeatIndicator {
                        opacity: .9
                        context: defaultContext
                    }
                    Waveform {
                        opacity: .9
                        context: defaultContext
                    }
                    Spectrum {
                        opacity: .9
                        context: defaultContext
                    }
                }
                RowLayout {
                    LibraryWidget {
                        id: libraryWidget
                        graph: graph;
                        registry: registry
                        context: defaultContext
                        width: 130
                        
                        Layout.fillHeight: true
                    }
                    Rectangle {
                        width: 1
                        Layout.fillHeight: true
                        Layout.topMargin: 20
                        Layout.bottomMargin: 20
                        color: "#eee"
                        opacity: 0.1
                    }
                }
            }

            Label {
                id: messages
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 7
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

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: {
            saveAction.trigger()
            Qt.quit()
        }
    }

    onClosing: {
        quitAction.trigger();
    }
}
