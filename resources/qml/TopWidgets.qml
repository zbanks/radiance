import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

RowLayout {
    property alias outputVisibleChecked: outputVisible.checked;
    property alias screenSelected: screenSelector.currentText;
    property alias midiDeviceSelected: midiSelector.currentIndex;

    ColumnLayout {
        Waveform {
            Layout.fillHeight: true;
        }
        Spectrum {
            Layout.fillHeight: true;
        }
    }
    ColumnLayout {
        GroupBox {
            title: "Output to screen"
            Layout.fillHeight: true;
            Layout.preferredWidth: 300;
            RowLayout {
                anchors.fill: parent;
                CheckBox {
                    id: outputVisible;
                    text: "Show output";
                }
                ComboBox {
                    id: screenSelector;
                    model: output.availableScreens;
                }
            }
        }
        GroupBox {
            title: "MIDI input"
            Layout.preferredWidth: 300;
            Layout.fillHeight: true;
            ColumnLayout {
                anchors.fill: parent;
                ComboBox {
                    id: midiSelector;
                    model: midi.deviceList;
                    Layout.fillWidth: true;
                }
                Button {
                    text: "Reload MIDI";
                    onClicked : midi.reload();
                    Layout.fillWidth: true;
                }
            }
        }
    }
    GroupBox {
        title: "Output Lux Buses";
        Layout.preferredWidth: 300;
        Layout.fillHeight: true;
        ColumnLayout {
            anchors.fill: parent;
            ScrollView {
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
        Layout.preferredWidth: 300;
        Layout.fillHeight: true;

        ColumnLayout {
            anchors.fill: parent;
            ScrollView {
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
