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
        }

        UIEffectPanel {}
    }
}
