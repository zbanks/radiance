import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

GroupBox {
    property alias outputVisibleChecked: outputVisible.checked;
    property var outputWindow;

    title: "Output to screen"
    Layout.preferredWidth: 300;
    RowLayout {
        anchors.fill: parent;
        CheckBox {
            id: outputVisible;
            text: "Show output";
            onCheckedChanged: {
                if (checked) {
                    outputWindow.screen = screenSelector.currentText;
                }
            }
        }
        ComboBox {
            id: screenSelector;
            model: outputWindow.availableScreens;
        }
    }
}
