import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RowLayout {
    Component.onCompleted: UISettings.previewSize = "500x300";
    width: 500;
    height: 300;

    ListModel {
        id: effectBoxModel;
        ListElement {
            name: "1";
        }
        ListElement {
            name: "2";
        }
        ListElement {
            name: "3";
        }
        ListElement {
            name: "4";
        }
    }

    ListView {
        anchors.fill: parent;
        orientation: Qt.Horizontal
        model: effectBoxModel;
        highlight: Rectangle {color: "lightsteelblue"; radius: 4 }
        focus: true;
        interactive: false;

        Keys.onPressed: {
            if (event.key == Qt.Key_H)
                decrementCurrentIndex();
            else if (event.key == Qt.Key_L)
                incrementCurrentIndex();
        }

        Component {
            id: effectBox;
            GroupBox {
                height: parent.height;
                Keys.onPressed: {
                    if (event.key == Qt.Key_J)
                        slider.value -= 0.1;
                    else if (event.key == Qt.Key_K)
                        slider.value += 0.1;
                }

                ColumnLayout {
                    anchors.fill: parent;

                    Slider {
                        id: slider;
                        Layout.fillWidth: true;
                        minimumValue: 0;
                        maximumValue: 1;
                    }

                    Effect {
                        id: effect;
                        Layout.fillHeight: true;
                        Layout.fillWidth: true;
                        intensity: slider.value;
                        source: "../resources/effects/test.glsl";
                        previous: this;
                    }
                    
                    ComboBox {
                        id: effectName;
                        Layout.fillWidth: true;
                        editable: true;
                        model: ["test", "rjump", "purple"];
                    }
                }
            }
        }

        delegate: effectBox;
    }

}
