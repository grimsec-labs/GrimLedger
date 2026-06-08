import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ListView {
        anchors.fill: parent
        anchors.margins: 12
        model: vault.credentialSummaries()
        clip: true
        delegate: Frame {
            width: ListView.view.width
            padding: 8
            background: Rectangle { color: "#0e0e12"; border.color: "#331111"; radius: 4 }
            ColumnLayout {
                width: parent.width
                Label { text: modelData.label; color: "#cc2200"; font.bold: true }
                Label { text: modelData.username; color: "#d4d4dc"; font.family: "monospace" }
                Label { text: modelData.url; color: "#998877"; elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
