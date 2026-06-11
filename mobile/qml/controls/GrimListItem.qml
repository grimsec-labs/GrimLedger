import QtQuick
import QtQuick.Controls
import Theme 1.0

ItemDelegate {
    id: control

    property string subtitle: ""
    property bool subtitleMonospace: false
    property bool selected: false

    implicitHeight: Math.max(Theme.touchTarget, contentColumn.implicitHeight + Theme.spacingMd)

    contentItem: Column {
        id: contentColumn
        width: parent.width
        spacing: Theme.spacingXs

        Text {
            width: parent.width
            text: control.text
            color: control.selected ? Theme.accent : Theme.textBright
            font.family: Theme.monoFont
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            visible: subtitle.length > 0
            text: subtitle
            color: Theme.textMuted
            font.family: subtitleMonospace ? Theme.monoFont : Theme.uiFont
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        color: control.selected ? Theme.listItemSelectedBg : "transparent"
        border.left.color: control.selected ? Theme.accent : "transparent"
        border.left.width: control.selected ? 2 : 0
    }
}
