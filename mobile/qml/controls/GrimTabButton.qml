import QtQuick
import QtQuick.Controls
import Theme 1.0

TabButton {
    id: control

    implicitHeight: Theme.touchTarget

    contentItem: Text {
        text: control.text
        font.family: Theme.monoFont
        font.pixelSize: 13
        color: control.checked ? Theme.accent : Theme.textMuted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: control.checked ? Theme.listItemSelectedBg : "transparent"
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: control.checked ? 2 : 0
            color: Theme.accent
            visible: control.checked
        }
    }
}
