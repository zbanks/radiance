import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Text {
    color: "#ddd"
    elide: Text.ElideMiddle
    padding: 5
    style: Text.Raised
    styleColor: "#333"
    Rectangle {
      gradient: Gradient {
          GradientStop { position: 0.0; color: "#333" }
          GradientStop { position: 0.5; color: "#000" }
      }
      anchors.fill: parent
      anchors.leftMargin: -7
      anchors.rightMargin: -7
      radius: height / 2
      z: -1;
  }
}
