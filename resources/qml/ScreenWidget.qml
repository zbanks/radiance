import QtQuick 2.7
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import radiance 1.0

RowLayout {
    property alias outputVisibleChecked: outputVisible.checked;
    property var outputWindow;

//    title: "Output to screen"
//    Layout.preferredWidth: 300;
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
