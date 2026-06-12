import QtQuick
import QtQuick.Controls
import Theme 1.0

ItemDelegate {
    id: control

    property string subtitle: ""
    property bool subtitleMonospace: false
    property bool selected: false
    property bool multiSelected: false

    implicitHeight: Math.max(Theme.touchTarget, contentRow.implicitHeight + Theme.spacingMd)

    contentItem: Row {
        id: contentRow
        width: parent.width
        spacing: Theme.spacingSm

        Rectangle {
            visible: control.multiSelected !== undefined && control.multiSelected !== false
                     || false
            width: visible ? 22 : 0
            height: 22
            radius: 3
            anchors.verticalCenter: parent.verticalCenter
            color: control.multiSelected ? Theme.accent : "transparent"
            border.color: Theme.accent
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: control.multiSelected ? "✓" : ""
                color: Theme.textBright
                font.pixelSize: 14
                font.bold: true
            }
        }

        Column {
            id: contentColumn
            width: parent.width - (contentRow.children[0].visible ? 22 + Theme.spacingSm : 0)
            spacing: Theme.spacingXs

            Text {
                width: parent.width
                text: control.text
                color: control.selected || control.multiSelected ? Theme.accent : Theme.textBright
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
    }

    background: Rectangle {
        color: control.selected || control.multiSelected ? Theme.listItemSelectedBg : "transparent"
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: (control.selected || control.multiSelected) ? 2 : 0
            color: Theme.accent
            visible: control.selected || control.multiSelected
        }
    }
}
