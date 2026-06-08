import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grim

Item {
    id: notesRoot

    property int currentNoteId: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            GrimButton {
                text: "+ New"
                onClicked: {
                    currentNoteId = vault.createNote("Untitled")
                    titleField.text = "Untitled"
                    bodyField.text = ""
                }
            }
            GrimButton {
                text: "Save"
                primary: false
                onClicked: vault.saveNote(currentNoteId, titleField.text, bodyField.text)
            }
        }

        ListView {
            id: noteList
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            model: vault.notes
            clip: true
            spacing: 4
            delegate: ItemDelegate {
                width: ListView.view.width
                text: modelData.title
                font.family: Theme.monoFont
                highlighted: currentNoteId === modelData.id
                onClicked: {
                    currentNoteId = modelData.id
                    titleField.text = modelData.title
                    bodyField.text = vault.noteBody(modelData.id)
                }
                background: Rectangle {
                    radius: Theme.radius
                    color: parent.highlighted ? "#1a1010" : Theme.panel
                    border.color: parent.highlighted ? Theme.accent() : Theme.border
                }
            }
        }

        GrimTextField {
            id: titleField
            Layout.fillWidth: true
            placeholderText: "Title"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: bodyField
                wrapMode: TextArea.Wrap
                color: Theme.text
                font.family: Theme.monoFont
                selectByMouse: true
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.panelAlt
                    border.color: activeFocus ? Theme.accent() : Theme.border
                    border.width: 1
                }
            }
        }
    }

    Component.onCompleted: {
        if (vault.notes.length > 0) {
            currentNoteId = vault.notes[0].id
            titleField.text = vault.notes[0].title
            bodyField.text = vault.noteBody(currentNoteId)
        }
    }
}
