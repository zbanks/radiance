import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

ApplicationWindow {
    id: window;
    visible: true;

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

    GridLayout {
        Component.onCompleted: UISettings.previewSize = "100x100";
        anchors.fill: parent;
        columns: 3;

        UIEffectSet { count: 4;}

        GroupBox {
            //width: 200;
            Layout.fillWidth: true;
            Layout.rowSpan: 2;

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

                Rectangle {
                    Layout.preferredHeight: width;
                    Layout.fillWidth: true;
                    color: "gray";
                }
            }
        }

        UIEffectSet { count: 4; layout: Qt.RightToLeft }
        UIEffectSet { count: 4; }
        UIEffectSet { count: 4; layout: Qt.RightToLeft }
    }
}
