import QtQuick
import QtQuick.Controls
import Theme 1.0
import controls 1.0

Item {
    ListView {
        id: credentialList
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        model: vault.credentialSummaries()
        clip: true
        spacing: Theme.spacingXs

        background: Rectangle {
            color: Theme.listBackground
            border.color: Theme.border
            border.width: 1
            radius: Theme.radiusSmall
        }

        delegate: GrimListItem {
            width: ListView.view.width
            text: modelData.label
            subtitle: modelData.username + (modelData.url ? " · " + modelData.url : "")
            subtitleMonospace: true
        }
    }

    Label {
        anchors.centerIn: parent
        visible: credentialList.count === 0
        text: "> no vault keys"
        color: Theme.textDim
        font.family: Theme.monoFont
        font.pixelSize: 13
    }
}
