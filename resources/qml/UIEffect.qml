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
        color: "blue";
    }

    ColumnLayout {
        anchors.fill: parent;

        Label {
            text: effect.source;
        }

/*
        ShaderEffect {
            Layout.fillWidth: true;
            Layout.preferredHeight: width;
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
                uniform sampler2D src;
                uniform lowp float qt_Opacity;
                void main() {
                    lowp vec4 tex = texture2D(src, coord);
                    gl_FragColor = vec4(vec3(dot(tex.rgb,
                                        vec3(0.344, 0.5, 0.156))),
                                             tex.a) * qt_Opacity;
                }"
            property variant src: effect;
        }
*/

        Effect {
            id: effect;
            Layout.preferredHeight: width;
            Layout.fillWidth: true;
            intensity: slider.value;
        }

        Slider {
            id: slider;
            Layout.fillWidth: true;
            minimumValue: 0;
            maximumValue: 1;
        }
    }
}
