import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RowLayout {
    property int layout: Qt.LeftToRight;
    layoutDirection: layout;

    Repeater {
        id: repeater;
        model: 4;
        GroupBox {
            property alias effect: effect;
            Layout.fillWidth: true;

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
                    Layout.preferredHeight: width;
                    Layout.fillWidth: true;
                    intensity: slider.value;
                    //source: "../resources/effects/" + effectName.currentText + ".glsl";
                    source: "../resources/effects/test.glsl";
                    previous: index == 0 ? null : repeater.itemAt(index - 1).effect;
                }
                
                ComboBox {
                    id: effectName;
                    Layout.fillWidth: true;
                    editable: true;
                    model: ["test " + index, "rjump", "purple", index];
                }
            }
        }
    }
}
