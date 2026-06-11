import QtQuick
import QtQuick.Controls
import Theme 1.0

CheckBox {
    id: control

    implicitHeight: Theme.touchTarget
    spacing: Theme.spacingSm

    contentItem: Text {
        text: control.text
        font.family: Theme.uiFont
        font.pixelSize: 14
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }

    indicator: Rectangle {
        implicitWidth: 22
        implicitHeight: 22
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: Theme.radiusSmall
        color: Theme.surface
        border.color: control.checked ? Theme.accent : Theme.borderInfernal
        border.width: 1

        Text {
            anchors.centerIn: parent
            visible: control.checked
            text: "\u2713"
            color: Theme.accent
            font.pixelSize: 14
            font.bold: true
        }
    }
}
