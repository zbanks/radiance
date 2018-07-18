import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls2
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
                    RadianceButton {
                        Item {
                            id: contentItem
                            property real oversample: 4
                            anchors.fill: parent
                            anchors.margins: 3
                            scale: 1. / oversample
                            Image {
                                id: image
                                anchors.centerIn: parent
                                source: "../graphics/gear.svg"
                                fillMode: Image.PreserveAspectFit
                                visible: false
                                width: parent.width * contentItem.oversample
                                height: parent.height * contentItem.oversample
                                sourceSize.height: 128
                            }
                            ColorOverlay {
                                anchors.fill: image
                                source: image
                                color: RadianceStyle.tileLineColor
                            }
                        }
                        text: ""
                        onClicked: settingsWidget.toggle()
                        implicitHeight: 25
                        implicitWidth: implicitHeight
                        colorDark: RadianceStyle.mainBackgroundColor
                    }
                    Item {
                        width: 1
                    }
                }
                SplitView {
                    id: splitView
                    property real openWidth: 150
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    LibraryWidget {
                        id: libraryWidget
                        graph: graph;
                        registry: registry
                        context: defaultContext
                        implicitWidth: splitView.openWidth
                        clip: true
                        
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.fill: parent
                            z: -1
                            opacity: 0.9
                            color: RadianceStyle.mainBackgroundColor
                        }

                        onSearchStarted: {
                            if (libraryWidget.width == 0) {
                                libraryWidget.width = splitView.openWidth;
                            }
                        }
                        onSearchStopped: {
                            if (libraryWidget.width != 0) {
                                splitView.openWidth = libraryWidget.width;
                                libraryWidget.width = 0;
                            }
                        }

                        Behavior on width {
                            enabled: !splitView.resizing
                            PropertyAnimation {
                                easing.type: Easing.InOutQuad;
                                duration: 300;
                            }
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Item {
                            id: settingsWidget
                            property real openWidth: 200

                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 0
                            clip: true

                            Behavior on width {
                                enabled: !splitView.resizing
                                PropertyAnimation {
                                    easing.type: Easing.InOutQuad;
                                    duration: 300;
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                z: -1
                                opacity: 0.9
                                color: RadianceStyle.mainBackgroundColor
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                color: RadianceStyle.mainLineColor
                                width: 1
                            }

                            SettingsWidget {
                                id: settings
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.openWidth - 20
                                anchors.margins: 10
                            }

                            function toggle() {
                                if (settingsWidget.width != 0) {
                                    settingsWidget.width = 0;
                                } else {
                                    settingsWidget.width = settingsWidget.openWidth;
                                }
                            }
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
