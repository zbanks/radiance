import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

ApplicationWindow {
    id: window;
    visible: true;

    Component.onCompleted: {
        UISettings.previewSize = "300x300";
        UISettings.outputSize = "1024x768";
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }

    Output {
        id: output;
        source: cross.crossfader;
        visible: controls.outputVisibleChecked;
        screen: controls.screenSelected;

        onVisibleChanged: controls.outputVisible.checked = visible;
    }

    MidiDevice {
        id: midi;
        deviceIndex: controls.midiDeviceSelected;
        onNoteOn: console.log("@" + ts + "channel: " + channel + "note on: " + note + "=" + velocity);
        onNoteOff: console.log("@" + ts + "channel: " + channel + "note on: " + note + "=" + velocity);
        onNoteAftertouch: console.log("@" + ts + "channel: " + channel + "note on: " + note + "=" + velocity);
        onControlChange: console.log("@" + ts + "channel: " + channel + "cc: " + control + "=" + value);
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
        layer.enabled: true;
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

        TopWidgets {
            Layout.maximumHeight: 200;
            id: controls;
        }

        Item {
            Layout.fillWidth: true;
            Layout.fillHeight: true;
            id: space;

            // Cant use Repeater on Deck :(
            Deck {
                id: deck1;
                count: 8;
            }
            Deck {
                id: deck2;
                count: 8;
            }
            Deck {
                id: deck3;
                count: 8;
            }
            Deck {
                id: deck4;
                count: 8;
            }

            RowLayout {
                ColumnLayout {
                    Repeater {
                        id: decks;
                        model: [deck1, deck2, deck3, deck4];

                        RowLayout {
                            Repeater {
                                id: effectSlots;
                                model: modelData.count;
                                property var deck: modelData;

                                EffectSlot {
                                    Layout.preferredWidth: 100;
                                    Layout.preferredHeight: 150;
                                    effectSelector: selector;
                                    onUiEffectChanged: {
                                        effectSlots.deck.set(index, (uiEffect == null) ? null : uiEffect.effect);
                                    }
                                }
                            }
                        }
                    }
                }

                UICrossFader {
                    id: cross;
                    crossfader.left: deck1.output;
                    crossfader.right: deck2.output;
                }
            }
            EffectSelector {
                id: selector;
            }
        }
    }
}
