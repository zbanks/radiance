import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.1
import QtQuick.Dialogs 1.2
import radiance 1.0
import "."

ApplicationWindow {
    id: window
    title: "GLSL Editor"
    property string file
    property var afterSave
    property int line
    property int col
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
                action: revertAction
            }
            MenuSeparator { }
            MenuItem {
                action: closeAction
            }
        }
    }

    statusBar: StatusBar {
        RowLayout {
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
        id: revertAction
        text: "Revert"
        shortcut: "Ctrl+R"
        onTriggered: {
            revert();
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
        wrapMode: TextEdit.NoWrap

        Keys.onPressed: {
            if (event.key == Qt.Key_Tab) {
                textArea.remove(textArea.selectionStart, textArea.selectionEnd);
                textArea.insert(textArea.cursorPosition, "&nbsp;&nbsp;&nbsp;&nbsp;");
                event.accepted = true;
            }
        }
    }

    GlslDocument {
        id: glslDocument
        document: textArea.textDocument
    }

    GlslHighlighter {
        document: textArea.textDocument
    }

    function load() {
        if (glslDocument.load(window.file)) {
            setCursor();
        }
    }

    function setCursor() {
        var pos = glslDocument.cursorPositionAt(line, col);
        textArea.cursorPosition = pos;
    }

    function save() {
        if (glslDocument.save(window.file)) {
            window.saved();
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

    function open(file) {
        if (window.visible && file == window.file) {
            setCursor();
        } else {
            window.file = file;
            if (window.file) {
                load();
            }
            window.visible = true;
        }
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

    function revert() {
        glslDocument.revert(window.file);
        saved(); // Not actually but effectively
    }
}
