import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: notesRoot

    property int currentNoteId: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "+ New"
                onClicked: {
                    currentNoteId = vault.createNote("Untitled")
                    noteList.model = vault.noteSummaries()
                    titleField.text = "Untitled"
                    bodyField.text = ""
                }
            }
            Button {
                text: "Save"
                onClicked: vault.saveNote(currentNoteId, titleField.text, bodyField.text)
            }
        }

        ListView {
            id: noteList
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            model: vault.noteSummaries()
            clip: true
            delegate: ItemDelegate {
                width: ListView.view.width
                text: modelData.title
                onClicked: {
                    currentNoteId = modelData.id
                    titleField.text = modelData.title
                    bodyField.text = vault.noteBody(modelData.id)
                }
            }
        }

        TextField {
            id: titleField
            Layout.fillWidth: true
            placeholderText: "Title"
            color: "#e8e8ec"
            background: Rectangle { color: "#111114"; border.color: "#331111"; radius: 3 }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: bodyField
                wrapMode: TextArea.Wrap
                color: "#e8e8ec"
                background: Rectangle { color: "#111114"; border.color: "#331111"; radius: 3 }
            }
        }
    }

    Component.onCompleted: noteList.model = vault.noteSummaries()
}
