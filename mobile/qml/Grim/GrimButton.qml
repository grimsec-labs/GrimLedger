import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool primary: true

    font.family: Theme.monoFont
    font.bold: primary
    padding: 10

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.down && primary ? Theme.bg : (primary ? Theme.accent() : Theme.textMuted)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radius
        color: {
            if (control.down && primary)
                return Theme.accent()
            if (control.hovered)
                return primary ? "#2a1010" : "transparent"
            return primary ? "#1a0808" : "transparent"
        }
        border.color: control.hovered && !primary ? "#665555"
            : (primary ? Theme.accent() : Theme.borderMuted)
        border.width: 1
    }
}
