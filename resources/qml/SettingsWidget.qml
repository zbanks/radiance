import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQml.Models 2.2
import radiance 1.0
import "."

Item {
    id: settingsWidget
    property alias modelName: modelNameComboBox.currentText

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

            Label {
                text: "MIDI controller:"
                color: RadianceStyle.mainTextColor
            }

            Loader {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

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
                color: RadianceStyle.mainTextColor
            }

            ComboBox {
                id: modelNameComboBox
                editable: true
                model: ["gui", "cli"]
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

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
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
