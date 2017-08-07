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
        UISettings.outputSize = "1024x768";
        RenderContext.addRenderTrigger(window, radmodel, 0);
    }

    Model {
        id: radmodel;
        onVideoNodeAdded: {
            console.log("Added video node", videoNode);
        }
        onVideoNodeRemoved: {
            console.log("Removed video node", videoNode);
        }
        onEdgeAdded: {
            console.log("Added edge");
        }
        onEdgeRemoved: {
            console.log("Removed edge");
        }
        onGraphChanged: {
            console.log("Graph Changed");
        }
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

        onVisibleChanged: controls.outputVisibleChecked = visible;
    }

    MidiDevice {
        id: midi;
        deviceIndex: controls.midiDeviceSelected;
        property var sliderMap; // [row, col] -> QSlider
        property var ghostMap;  // [row, col] -> QSlider
        property var ccMap;     // [channel, control] -> [row, col]
        property var ccState;   // [channel, control] -> value; value \in [0.0, 1.0]

        Component.onCompleted: {
            sliderMap = {};
            ghostMap = {};
            ccMap = {};
            ccState = {};
        }

        onNoteOn: console.log("MIDI note: channel: " + channel + "; note on: " + note + "=" + velocity);
        onNoteOff: console.log("MIDI note: channel: " + channel + "; note off: " + note + "=" + velocity);
        onNoteAftertouch: console.log("MIDI note: channel: " + channel + "; note at: " + note + "=" + velocity);
        onControlChange: {
            console.log("MIDI CC: channel: " + channel + "; cc: " + control + "=" + value);
            var v = value / 127.0;
            var oldV = ccState[[channel, control]];
            if (oldV === undefined) {
                console.log("undef");
                oldV = v;
            }
            ccState[[channel, control]] = v;

            var rc = ccMap[[channel, control]];
            if (!rc) return;

            var ghost = ghostMap[rc];
            if (ghost)
                ghost.value = v;

            var slider = sliderMap[rc];
            if (slider) {
                var TOLERANCE = 1.5 / 127.0;
                if (slider.value >= Math.min(oldV, v) - TOLERANCE && slider.value <= Math.max(oldV, v) + TOLERANCE)
                    slider.value = v;
            }
        }

        function controlSlider(row, col, slider, ghost) {
            sliderMap[[row, col]] = slider;
            ghostMap[[row, col]] = ghost;

            // Samson Graphite; Preset #4
            // 1-8 are Channels 8-15
            // Knob: CC 10
            // Slider: CC 7
            // Mute: CC 16
            // Rec: CC 18
            var channel = null, control = null;
            if (row == 0) {
                channel = 8+col;
                control = 10;
            } else if (row == 1) {
                channel = 8+col;
                control = 7;
            }
            ccMap[[channel, control]] = [row, col];

            var v = ccState[[channel, control]];
            if (v)
                ghost.value = v;
        }
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
            Layout.bottomMargin: 30;
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
                            property int row: index;

                            Repeater {
                                id: effectSlots;
                                model: modelData.count;
                                property var deck: modelData;

                                EffectSlot {
                                    property int col: index;

                                    Layout.preferredWidth: 100;
                                    Layout.preferredHeight: 170;
                                    effectSelector: selector;
                                    onUiEffectChanged: {
                                        if (uiEffect == null) {
                                            effectSlots.deck.set(index, null);
                                            midi.controlSlider(row, col, null, null);
                                        } else {
                                            effectSlots.deck.set(index, uiEffect.effect);
                                            midi.controlSlider(row, col, uiEffect.slider, uiEffect.sliderGhost);
                                        }
                                    }
                                }
                            }

                            Slider {
                                id: deckVolume;
                                Layout.fillHeight: true;
                                orientation: Qt.Vertical;
                                minimumValue: 0;
                                maximumValue: 1;
                            }
                        }
                    }
                }

                //UICrossFader {
                //    id: cross;
                //    crossfader.left: deck1.output;
                //    crossfader.right: deck2.output;
                //}
            }
            EffectSelector {
                id: selector;
            }
        }
    }
}
