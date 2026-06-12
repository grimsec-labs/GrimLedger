import QtQuick
import QtQuick.Controls
import Theme 1.0

TextArea {
    id: control

    property bool useWordWrap: true

    padding: Theme.spacingSm
    color: Theme.textBright
    placeholderTextColor: Theme.textMuted
    font.family: Theme.uiFont
    font.pixelSize: 15
    wrapMode: useWordWrap ? TextArea.Wrap : TextArea.NoWrap
    selectionColor: Theme.selectionBg
    selectedTextColor: Theme.selectionText

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.surface
        border.color: control.activeFocus ? Theme.accent : Theme.borderInfernal
        border.width: 1
    }
}
