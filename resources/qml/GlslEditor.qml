import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.1
import QtQuick.Dialogs 1.3
import radiance 1.0
import "."

ApplicationWindow {
    id: window
    title: "GLSL Editor"
    property string file
    property var afterSave
    signal saved()

    menuBar: MenuBar {
        Menu {
            title: "&File"
            MenuItem {
                action: newAction
            }
            MenuItem {
                action: openAction
            }
            MenuItem {
                action: saveAction
            }
            MenuItem {
                action: saveAsAction
            }
            MenuSeparator { }
            MenuItem {
                action: closeAction
            }
        }
    }

    statusBar: StatusBar {
        RowLayout {
            //anchors.fill: parent
            Label { text: glslDocument.message }
        }
    }

    Action {
        id: newAction
        text: "&New..."
        shortcut: "Ctrl+N"
        onTriggered: {
            tryClear();
        }
    }

    Action {
        id: openAction
        text: "&Open..."
        shortcut: "Ctrl+O"
        onTriggered: {
            showOpenDialog();
        }
    }

    Action {
        id: saveAction
        text: "&Save"
        shortcut: "Ctrl+S"
        onTriggered: {
            if (window.file) {
                save();
            } else {
                showSaveDialog();
            }
        }
    }

    Action {
        id: saveAsAction
        text: "Save &As..."
        onTriggered: {
            showSaveDialog();
        }
    }

    Action {
        id: closeAction
        text: "E&xit"
        shortcut: "Ctrl+Q"
        onTriggered: {
            tryClose()
        }
    }

    onClosing: {
        closeAction.trigger();
        close.accepted = false;
    }

    function urlToPath(url) {
        var path = url.toString();
        // remove prefixed "file://"
        path = path.replace(/^(file:\/{2})/,"");
        return path;
    }

    function showOpenDialog() {
        openDialog.folder = glslDocument.loadDirectory(window.file);
        openDialog.open();
    }

    function showSaveDialog() {
        saveAsDialog.folder = glslDocument.saveDirectory(window.file);
        saveAsDialog.open();
    }

    FileDialog {
        id: saveAsDialog
        title: "Save As"
        selectExisting: false
        nameFilters: [ "GLSL files (*.glsl)", "All files (*)" ]
        onAccepted: {
            var path = urlToPath(saveAsDialog.fileUrl);
            if (path) {
                window.file = glslDocument.contractLibraryPath(path);
                save();
            }
            afterSave = null;
        }
        onRejected: {
            afterSave = null;
        }
    }

    FileDialog {
        id: openDialog
        title: "Open"
        nameFilters: [ "GLSL files (*.glsl)", "All files (*)" ]
        onAccepted: {
            var path = urlToPath(openDialog.fileUrl);
            if (path) {
                window.file = glslDocument.contractLibraryPath(path);
                load();
            }
        }
        onRejected: {
        }
    }

    MessageDialog {
        id: saveChangesBeforeClose
        title: "Save changes?"
        icon: StandardIcon.Question
        text: window.file ? "Save changes to \"" + window.file + "\" before closing?" : "Save changes before closing?"
        standardButtons: StandardButton.Discard | StandardButton.Cancel | StandardButton.Save
        onDiscard: {
            window.close();
        }
        onAccepted: {
            afterSave = window.close;
            saveAction.trigger();
        }
    }

    MessageDialog {
        id: saveChangesBeforeClear
        title: "Save changes?"
        icon: StandardIcon.Question
        text: window.file ? "Save changes to \"" + window.file + "\" before clearing?" : "Save changes before clearing?"
        standardButtons: StandardButton.Discard | StandardButton.Cancel | StandardButton.Save
        onDiscard: {
            window.clear();
        }
        onAccepted: {
            afterSave = window.clear;
            saveAction.trigger();
        }
    }

    TextArea {
        id: textArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        textFormat: Qt.RichText
        Component.onCompleted: forceActiveFocus()
        font.family: "Monospace"
        style: TextAreaStyle {
            textColor: RadianceStyle.editorTextColor
            backgroundColor: RadianceStyle.editorBackgroundColor
        }
        frameVisible: false
    }

    GlslDocument {
        id: glslDocument
        document: textArea.textDocument
    }

    function load() {
        if (glslDocument.load(window.file)) {
        }
    }

    function save() {
        if (glslDocument.save(window.file)) {
            saved();
            if (afterSave) {
                afterSave();
            }
        }
        afterSave = null;
    }

    function close() {
        window.visible = false;
    }

    function clear() {
        window.file = ""
        glslDocument.clear();
    }

    function open() {
        window.visible = true;
        window.afterSave = null;
    }

    function tryClear() {
        if (glslDocument.modified) {
            saveChangesBeforeClear.open();
        } else {
            clear();
        }
    }

    function tryClose() {
        if (glslDocument.modified) {
            saveChangesBeforeClose.open();
        } else {
            close();
        }
    }
}
