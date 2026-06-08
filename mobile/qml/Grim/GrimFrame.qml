import QtQuick
import QtQuick.Controls

Frame {
    id: control
    property bool highlighted: false
    padding: 10

    background: Rectangle {
        radius: Theme.radius + 1
        color: control.highlighted ? "#1a1010" : Theme.panel
        border.color: control.highlighted ? Theme.accent() : Theme.border
        border.width: 1
    }
}
