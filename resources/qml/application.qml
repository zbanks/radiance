import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0
import "."

ApplicationWindow {
    id: window;
    visible: true
    color: RadianceStyle.mainBackgroundColor
    width: 800
    height: 530
    title: "Radiance"
    property bool hasMidi: false
    property string modelName: settings.modelName

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

    Timer {
        repeat: true
        running: true
        interval: 10 * 1000
        onTriggered: save()
    }

    Component.onCompleted: {
        Globals.previewAdapter = previewAdapter;

        load();
        if (model.vertices.length == 0) {
            // If the state was empty, then open up a few nodes as a demo
            model.load(defaultContext, registry, "gui_default");
        }
    }

    ColumnLayout {
        anchors.fill: parent

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
                spacing: 10

                RowLayout {
                    spacing: 20

                    Item {
                        width: 1
                    }
                    Waveform {
                        opacity: .9
                        context: defaultContext
                    }
                    BeatIndicator {
                        opacity: .9
                        context: defaultContext
                    }
                    Spectrum {
                        opacity: .9
                        context: defaultContext
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    PopOut {
                        id: libraryContainer
                        openWidth: 150
                        side: "left"
                        open: true

                        LibraryWidget {
                            id: libraryWidget
                            graph: graph;
                            registry: registry
                            context: defaultContext

                            anchors.fill: parent

                            onSearchStarted: {
                                libraryContainer.open = true
                            }
                            onSearchStopped: {
                                libraryContainer.open = false
                            }
                        }
                    }
                    PopOut {
                        id: settingsContainer
                        side: "right"

                        SettingsWidget {
                            id: settings
                            anchors.fill: parent
                        }
                    }
                }
            }

            Label {
                id: messages
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 7
                color: RadianceStyle.mainTextColor
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        messages.text = "";
                    }
                }
            }
        }
    }

    function save() {
        if (model.vertices.length >= 0) {
            model.save(modelName);
        }
    }

    function load() {
        console.log("Loading state from file...");
        model.load(defaultContext, registry, modelName);
        model.flush();
    }

    function quit() {
        save()
        Qt.quit()
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: save()
    }

    Shortcut {
        sequence: "Ctrl+R"
        onActivated: load()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: quit()
    }

    onClosing: quit()
}
