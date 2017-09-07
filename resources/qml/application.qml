import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import radiance 1.0

ApplicationWindow {
    id: window;
    visible: true;

    Model {
        id: model;
        onGraphChanged: {
            var changeset = "+" + verticesAdded.length + " -" + verticesRemoved.length + " vertices, ";
            changeset += "+" + edgesAdded.length + " -" + edgesRemoved.length + " edges";
            console.log("Graph changed!", changeset);
        }
    }

    EffectNode {
        id: en
        name: "test"
    }
    EffectNode {
        id: en2
        name: "hue"
    }
    EffectNode {
        id: en3
        name: "tunnel"
    }
    EffectNode {
        id: en4
        name: "yellow"
    }

    ImageNode {
        id: img1
        imagePath: "nyancat.gif"
        inputCount: 1 // FIXME: this should be 0
    }

    EffectNode {
        id: cross
        //name: "crossfader"
        name: "greenscreen"
        inputCount: 2
    }
    EffectNode {
        id: after
        name: "pixelate"
    }

    /*
    EffectNode {
        id: rgbmask
        name: "rgbmask"
        inputCount: 4
    }
    */

    Component.onCompleted: {
        UISettings.previewSize = "100x100";
        UISettings.outputSize = "1024x768";
        model.addVideoNode(en);
        model.addVideoNode(en2);
        model.addVideoNode(en3);
        model.addVideoNode(img1);

        /*
        model.addVideoNode(rgbmask);
        model.addEdge(en, rgbmask, 0);
        model.addEdge(en2, rgbmask, 1);
        model.addEdge(en3, rgbmask, 2);
        model.addEdge(en4, rgbmask, 3);
        */

        console.log("Crossfader:");
        model.addVideoNode(cross);
        //model.addVideoNode(after);
        model.addVideoNode(en4);
        model.addEdge(en, en2, 0);
        model.addEdge(en2, en3, 0);
        model.addEdge(en3, cross, 0);
        //model.addEdge(img1, en4, 0);
        model.addEdge(en4, cross, 1);
        //model.addEdge(cross, after, 0);
        model.flush();
        RenderContext.addRenderTrigger(window, model, 0);
    }

    RowLayout {
        ColumnLayout {
            RowLayout {
                Waveform {

                }
                Spectrum {

                }
            }

            RowLayout {
                Layout.fillWidth: true;

                // This is kind of crappy, but it was easy 
                ComboBox {
                    id: effectSelector;
                    model: Object.keys(NodeRegistry.nodeTypes);
                    editable: true;
                    Layout.preferredWidth: 300;
                }
                Button {
                    text: "Add"
                    onClicked: {
                        var e = Qt.createQmlObject("import radiance 1.0; EffectNode { }", window, "");
                        e.name = effectSelector.currentText;
                        model.addVideoNode(e);
                        model.flush();
                        console.log("New Effect", effectSelector.currentText, e);
                    }
                }
            }

            View {
                model: model;
                delegates: {
                    "EffectNode": "EffectNodeTile",
                    "ImageNode": "ImageNodeTile",
                    "": "VideoNodeTile"
                }
                width: 800
                height: 500
            }
        }

        Rectangle {
            color: "#000"
            width: 500
            height: 500
            VideoNodeRender {
                id: vnr
                anchors.fill: parent
                chain: 0
                videoNode: cross
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    vnr.update()
                    model.removeVideoNode(en4);
                    model.flush();
                }
            }
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        onTriggered: Qt.quit()
    }
}
