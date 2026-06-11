import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

Item {
    id: notesRoot

    property int currentNoteId: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        spacing: Theme.spacingSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            GrimButton {
                text: "+ New"
                onClicked: {
                    currentNoteId = vault.createNote("Untitled")
                    noteList.model = vault.noteSummaries()
                    titleField.text = "Untitled"
                    bodyField.text = ""
                }
            }

            Item { Layout.fillWidth: true }

            GrimButton {
                text: "Save"
                primary: true
                onClicked: vault.saveNote(currentNoteId, titleField.text, bodyField.text)
            }
        }

        Label {
            visible: noteList.count === 0
            text: "> no notes yet — create one"
            color: Theme.textDim
            font.family: Theme.monoFont
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            id: noteList
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            model: vault.noteSummaries()
            clip: true
            spacing: 1

            background: Rectangle {
                color: Theme.listBackground
                border.color: Theme.border
                border.width: 1
                radius: Theme.radiusSmall
            }

            delegate: GrimListItem {
                width: ListView.view.width
                text: modelData.title
                selected: currentNoteId === modelData.id
                onClicked: {
                    currentNoteId = modelData.id
                    titleField.text = modelData.title
                    bodyField.text = vault.noteBody(modelData.id)
                }
            }
        }

        GrimTextField {
            id: titleField
            Layout.fillWidth: true
            placeholderText: "Title"
            monospace: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GrimTextArea {
                id: bodyField
            }
        }
    }

    Component.onCompleted: noteList.model = vault.noteSummaries()
}
