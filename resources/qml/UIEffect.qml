import QtQuick 2.3
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import radiance 1.0

Item {
    layer.enabled: false;
    property alias effect: effect;
    width: 200;
    height: 300;

    Keys.onPressed: {
        if (event.key == Qt.Key_J)
            slider.value -= 0.1;
        else if (event.key == Qt.Key_K)
            slider.value += 0.1;
    }

    Rectangle {
        anchors.fill: parent;
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#222" }
            GradientStop { position: 1.0; color: "#111" }
        }
        radius: 10;
        border.width: 3;
        border.color: "#666";
    }

    ColumnLayout {
        anchors.fill: parent;
        anchors.margins: 15;

        Label {
            text: effect.source;
            color: "#ddd";
        }

        Item {
            Layout.preferredHeight: width;
            Layout.fillWidth: true;

            ShaderEffect {
                anchors.fill: parent;
                vertexShader: "
                    uniform highp mat4 qt_Matrix;
                    attribute highp vec4 qt_Vertex;
                    attribute highp vec2 qt_MultiTexCoord0;
                    varying highp vec2 coord;
                    void main() {
                        coord = qt_MultiTexCoord0;
                        gl_Position = qt_Matrix * qt_Vertex;
                    }"
                fragmentShader: "
                    varying highp vec2 coord;
                    uniform lowp float qt_Opacity;
                    void main() {
                        vec2 qc = floor(coord.xy * 4.);
                        vec3 c = vec3(0.2) + vec3(0.1) * mod(qc.x + qc.y, 2.);
                        gl_FragColor = vec4(c, 1) * qt_Opacity;
                    }"
            }

            Effect {
                id: effect;
                anchors.fill: parent;
                intensity: slider.value;
            }
        }

        Slider {
            id: slider;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
    }
}
