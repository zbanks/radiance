import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

RowLayout {
    Component.onCompleted: UISettings.previewSize = "300x300";
    width: 300;
    height: 300;

    GroupBox {
        anchors.fill: parent;
        ColumnLayout {
            anchors.fill: parent;

            Slider {
                id: slider;
                Layout.fillWidth: true;
                minimumValue: 0;
                maximumValue: 1;
            }

            Effect {
                Layout.fillHeight: true;
                Layout.fillWidth: true;
                intensity: slider.value;
                source: "../resources/effects/test.glsl";
                Component.onCompleted: {setPrevious(this);}
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
