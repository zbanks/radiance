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

    menuBar: MenuBar {
        Menu {
            title: "File";
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

    RowLayout {

        ColumnLayout {
            anchors.fill: parent;

            RowLayout {
                Layout.fillHeight: true;
                Rectangle {
                    width: 300;
                    Layout.fillHeight: true;
                    color: "white";
                    Label { text: "TODO: Waveform" }
                }
                Rectangle {
                    width: 300;
                    Layout.fillHeight: true;
                    color: "grey";
                    Label { text: "TODO: Spectrum" }
                }
                GroupBox {
                    title: "Outputs";
                    Layout.fillHeight: true;
                    Layout.fillWidth: true;

                    OutputManager {
                        id: outputManager
                        Component.onCompleted: { this.createBus("udp://127.0.0.1:1365"); }
                    }
                    ColumnLayout {
                        anchors.fill: parent;
                        ScrollView {
                            Layout.fillHeight: true;
                            Layout.fillWidth: true;
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

            //UIEffectPanel {}

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
                        onUiEffectChanged: {
                            //console.log("EFFECT", uiEffect, uiEffect.effect, effect);
                            deck2.set(deck2repeater.count - index - 1, (uiEffect == null) ? null : uiEffect.effect);
                        }
                    }

                    Component.onCompleted: {
                        itemAt(2).load("test");
                        itemAt(1).load("circle");
                    }
                }
            }
        }
    }
}
