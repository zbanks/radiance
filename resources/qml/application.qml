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
                title: "Outputs";
                implicitWidth: 300;
                implicitHeight: 300;
                Layout.fillWidth: true;

                OutputManager {
                    id: outputManager
                    Component.onCompleted: { this.createBus("udp://127.0.0.1:1365"); }
                }
                ColumnLayout {
                    ScrollView {
                        ListView {
                            model: outputManager.buses
                            delegate: RowLayout {
                                property var bus : outputManager.buses[index];
                                TextField {
                                    id: bus_textbox;
                                    text: bus.uri;
                                    onAccepted: { bus.uri = text }
                                }
                                Label { text: "Bus URI: " + bus.uri + "; State: " + bus.state}
                            }
                        }
                    }
                    Button {
                        text: "Add Output";
                        onClicked: { outputManager.createBus(""); }
                    }
                }
            }
        }

        EffectSpace {
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
                        effectSpace: space;
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
                        effectSpace: space;
                        onUiEffectChanged: {
                            deck2.set(deck2repeater.count - index - 1, (uiEffect == null) ? null : uiEffect.effect);
                        }
                    }
                }
            }
        }
    }
}
