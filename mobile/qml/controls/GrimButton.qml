import QtQuick
import QtQuick.Controls
import Theme 1.0

Button {
    id: control

    property bool primary: false

    implicitHeight: Theme.touchTarget
    padding: Theme.spacingSm

    contentItem: Text {
        text: control.text
        font.family: Theme.uiFont
        font.pixelSize: 14
        font.bold: control.primary
        color: control.primary ? Theme.accent : Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: {
            if (!control.enabled)
                return Theme.surface
            if (control.primary)
                return control.pressed ? Theme.primaryButtonPressedBg : Theme.primaryButtonBg
            return control.pressed ? Theme.listItemSelectedBg : Theme.surface
        }
        border.color: {
            if (control.primary)
                return control.pressed ? Theme.accentHover : Theme.accent
            return Theme.border
        }
        border.width: 1
    }
}
