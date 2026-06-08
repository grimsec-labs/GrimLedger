import QtQuick
import QtQuick.Controls

TextField {
    id: control
    color: Theme.text
    font.family: Theme.monoFont
    padding: 10
    selectByMouse: true

    placeholderTextColor: Theme.textDim

    background: Rectangle {
        radius: Theme.radius
        color: Theme.panelAlt
        border.color: control.activeFocus ? Theme.accent() : Theme.border
        border.width: 1
    }
}
