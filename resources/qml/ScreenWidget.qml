import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

GroupBox {
    property alias outputVisibleChecked: outputVisible.checked;
    property alias screenSelected: screenSelector.currentText;
    property var outputWindow;

    title: "Output to screen"
    Layout.preferredWidth: 300;
    RowLayout {
        anchors.fill: parent;
        CheckBox {
            id: outputVisible;
            text: "Show output";
        }
        ComboBox {
            id: screenSelector;
            model: outputWindow.availableScreens;
        }
    }
}
