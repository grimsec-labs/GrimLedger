import QtQuick
import QtQuick.Controls
import Theme 1.0

TextField {
    id: control

    property bool passwordMode: false
    property bool monospace: false

    implicitHeight: Theme.touchTarget
    padding: Theme.spacingSm
    color: Theme.textBright
    placeholderTextColor: Theme.textMuted
    font.family: monospace ? Theme.monoFont : Theme.uiFont
    font.pixelSize: 14
    echoMode: passwordMode ? TextInput.Password : TextInput.Normal
    selectionColor: Theme.selectionBg
    selectedTextColor: Theme.selectionText

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.borderInfernal
        border.width: 1
    }
}
