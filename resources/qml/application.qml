import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

ApplicationWindow {
    id: window;
    visible: true;

    Component.onCompleted: {
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "640x480";
        var component = Qt.createComponent("OutputWindow.qml")
        var window = component.createObject(window)
        window.source = cross.crossfader;
        window.show()
    }
    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }
    menuBar: MenuBar {
        Menu {
            title: "&File";
            MenuItem { action: quitAction }
        }
        Menu {
            title: "&Edit"
        }
    }

    statusBar: StatusBar {
        RowLayout {
            anchors.fill: parent;
            Label { text: "Live" }
        }
    }

    LinearGradient {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#333" }
            GradientStop { position: 1.0; color: "#444" }
        }
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 10;
        Layout.margins: 10;

        RowLayout {
            Rectangle {
                implicitWidth: 300;
                implicitHeight: 300;
                Layout.fillWidth: true;
                color: "white";
                Label { text: "TODO: Waveform" }
            }
            Rectangle {
                implicitWidth: 300;
                implicitHeight: 300;
                Layout.fillWidth: true;
                color: "grey";
                Label { text: "TODO: Spectrum" }
            }
            GroupBox {
                title: "Output Lux Buses";
                implicitWidth: 200;
                implicitHeight: 300;
                Layout.fillWidth: true;

                ColumnLayout {
                    anchors.fill: parent;
                    ScrollView {
                        Layout.fillWidth: true;
                        ListView {
                            model: OutputManager.buses
                            delegate: RowLayout {
                                property var bus : OutputManager.buses[index];
                                Rectangle {
                                    width: 15;
                                    height: 15;
                                    color: bus.state == LuxBus.Disconnected ? "grey" : (
                                           bus.state == LuxBus.Error ? "red" : (
                                           bus.state == LuxBus.Connected ? "green" : "blue"));
                                }
                                TextField {
                                    id: bus_textbox;
                                    text: bus.uri;
                                    implicitWidth: 200;
                                    Layout.margins: 3;
                                    onAccepted: { bus.uri = text }
                                }
                                //Label { text: "Bus URI: " + bus.uri + "; State: " + bus.state }
                            }
                        }
                    }
                    RowLayout {
                        Button {
                            text: "Add Bus";
                            onClicked: {
                                var luxbus = OutputManager.createLuxBus();
                                luxbus.uri = "udp://127.0.0.1:1365";
                            }
                        }
                        Button {
                            text: "Refresh";
                            onClicked: {
                                OutputManager.refresh();
                            }
                        }
                        Button {
                            text: "Save";
                            onClicked: {
                                OutputManager.saveSettings();
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: "Output Lux Devices";
                implicitWidth: 200;
                implicitHeight: 300;
                Layout.fillWidth: true;

                ColumnLayout {
                    anchors.fill: parent;
                    ScrollView {
                        Layout.fillWidth: true;
                        ListView {
                            model: OutputManager.devices
                            delegate: RowLayout {
                                property var dev : OutputManager.devices[index];
                                Rectangle {
                                    width: 15;
                                    height: 15;
                                    color: dev.state == LuxDevice.Disconnected ? "grey" : (
                                           dev.state == LuxDevice.Error ? "red" : (
                                           dev.state == LuxDevice.Blind ? "yellow" : (
                                           dev.state == LuxDevice.Connected ? "green" : "blue")));
                                }
                                Rectangle {
                                    width: 15;
                                    height: 15;
                                    radius: 8;
                                    color: dev.color;
                                }
                                TextField {
                                    id: dev_name;
                                    text: dev.name;
                                    implicitWidth: 100;
                                    Layout.margins: 3;
                                    onAccepted: { dev.name = text }
                                }
                                TextField {
                                    id: dev_id;
                                    text: "0x" + dev.luxId.toString(16);
                                    implicitWidth: 100;
                                    Layout.margins: 3;
                                    onAccepted: { dev.luxId = parseInt(text) }
                                }
                                Label {
                                    text: "Length: " + dev.length;
                                }
                            }
                        }
                    }
                    RowLayout {
                        Button {
                            text: "Add Device";
                            onClicked: {
                                var luxdev = OutputManager.createLuxDevice();
                            }
                        }
                        Button {
                            text: "Refresh";
                            onClicked: {
                                OutputManager.refresh();
                            }
                        }
                        Button {
                            text: "Save";
                            onClicked: {
                                OutputManager.saveSettings();
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true;
            Layout.fillHeight: true;
            id: space;

            RowLayout {
                Deck {
                    id: deck1;
                    count: 4;
                }

                Deck {
                    id: deck2;
                    count: 4;
                }
                Repeater {
                    id: deck1repeater;
                    model: deck1.count;

                    EffectSlot {
                        effectSelector: selector;
                        onUiEffectChanged: {
                            deck1.set(index, (uiEffect == null) ? null : uiEffect.effect);
                        }
                    }
                }

                UICrossFader {
                    id: cross;
                    crossfader.left: deck1.output;
                    crossfader.right: deck2.output;
                }

                Repeater {
                    id: deck2repeater;
                    model: deck2.count;

                    EffectSlot {
                        effectSelector: selector;
                        onUiEffectChanged: {
                            deck2.set(deck2repeater.count - index - 1, (uiEffect == null) ? null : uiEffect.effect);
                        }
                    }
                }
            }

            EffectSelector {
                id: selector;
            }
        }
        /*
        Output {
            width: 500;
            height: 500;
            source: cross.crossfader;
        }*/
    }
}
