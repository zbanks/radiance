import QtQuick 2.7
import Qt.labs.folderlistmodel 2.1
import radiance 1.0

ComboBox {
    id: combobox
    property var target;
    property string qmlDir;
    implicitWidth: 120
    currentIndex: 0

    model: folderModel
    textRole: "fileName"

    onCurrentTextChanged: {
        loader.source = folderModel.get(currentIndex, "filePath")
    }

    FolderListModel {
        id: folderModel
        folder: "MidiMappings"
        showDirs: false
        nameFilters: ["*.qml"]
    }
    Loader {
        id: loader
        onLoaded: {
            console.log("Loading mapping: " + source);
            item.target = target;
        }
    }
}
