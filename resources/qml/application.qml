import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

GridLayout {
    Component.onCompleted: UISettings.previewSize = "600x300";
    width: 600;
    height: 300;
    columns: 3;

    UIEffectSet { }

    Rectangle {
        width: 100;
        Layout.fillHeight: true;
        Layout.rowSpan: 2;
    }

    UIEffectSet { layout: Qt.RightToLeft }
    UIEffectSet { }
    UIEffectSet { layout: Qt.RightToLeft }
}
