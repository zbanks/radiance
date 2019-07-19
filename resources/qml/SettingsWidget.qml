import QtQuick 2.7
import QtQuick.Layouts 1.2
import radiance 1.0
import "."

Item {
    id: settingsWidget
    property alias modelName: modelNameTextField.text
    property var model;
    property var registry;

    ColumnLayout {
        anchors.fill: parent

        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "MIDI controller:"
                color: RadianceStyle.mainTextColor
            }

            Loader {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                Layout.fillWidth: true

                source: window.hasMidi ? "MidiMappingSelector.qml" : ""
                onLoaded: {
                    item.target = graph.view;
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

            Label {
                text: "Model File:"
            }

            TextField {
                id: modelNameTextField
                text: "model.json"
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

            Button {
                text: "Save"
                onClicked: save()
            }
            Button {
                text: "Load"
                onClicked: load()
            }
            Button {
                text: "Clear"
                onClicked: {
                    model.clear();
                    model.flush();
                }
            }
        }

        AutoDJ {
            context: defaultContext
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
