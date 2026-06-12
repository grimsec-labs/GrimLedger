import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Theme 1.0
import controls 1.0

Item {
    id: notesRoot

    property int currentNoteId: 0
    property var imagePreviews: []

    function refreshList() {
        if (searchField.text.length > 0)
            noteList.model = vault.searchNotes(searchField.text)
        else
            noteList.model = vault.noteSummaries()
    }

    function loadNotePreviews() {
        if (currentNoteId > 0 && vault.unlocked)
            imagePreviews = vault.noteImagePreviews(currentNoteId)
        else
            imagePreviews = []
    }

    function openNote(noteId, title) {
        currentNoteId = noteId
        titleField.text = title
        bodyField.text = vault.noteBody(noteId)
        loadNotePreviews()
    }

    function clearNote() {
        currentNoteId = 0
        titleField.text = ""
        bodyField.text = ""
        imagePreviews = []
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        spacing: Theme.spacingSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            GrimTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search notes..."
                monospace: true
                onTextChanged: refreshList()
            }

            GrimButton {
                text: "+ New"
                onClicked: {
                    currentNoteId = vault.createNote("Untitled")
                    refreshList()
                    titleField.text = "Untitled"
                    bodyField.text = ""
                    imagePreviews = []
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            GrimButton {
                text: "Delete"
                enabled: currentNoteId > 0
                onClicked: deleteNotePopup.open()
            }

            GrimButton {
                text: "Insert Image"
                visible: vault.unlocked
                enabled: currentNoteId > 0
                onClicked: imageFileDialog.open()
            }

            GrimButton {
                text: "Camera"
                visible: vault.unlocked && Qt.platform.os === "android"
                enabled: currentNoteId > 0
                onClicked: vault.launchCamera(currentNoteId)
            }

            Item { Layout.fillWidth: true }

            GrimButton {
                text: "Save"
                primary: true
                enabled: currentNoteId > 0
                onClicked: {
                    vault.saveNote(currentNoteId, titleField.text, bodyField.text)
                    refreshList()
                    loadNotePreviews()
                }
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            color: Theme.listBackground
            border.color: Theme.border
            border.width: 1
            radius: Theme.radiusSmall

            ListView {
                id: noteList
                anchors.fill: parent
                anchors.margins: 1
                model: vault.noteSummaries()
                clip: true
                spacing: 1

                delegate: GrimListItem {
                    width: ListView.view.width
                    text: modelData.title
                    selected: currentNoteId === modelData.id
                    onClicked: openNote(modelData.id, modelData.title)
                }
            }
        }

        GrimTextField {
            id: titleField
            Layout.fillWidth: true
            placeholderText: "Title"
            monospace: true
        }

        // Attachment image gallery — resolves from encrypted store on every note load
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: imageGrid.contentHeight + Theme.spacingSm * 2
            Layout.maximumHeight: 280
            visible: imagePreviews.length > 0
            color: Theme.listBackground
            border.color: Theme.borderInfernal
            border.width: 1
            radius: Theme.radiusSmall
            clip: true

            Flickable {
                id: imageGrid
                anchors.fill: parent
                anchors.margins: Theme.spacingSm
                contentWidth: width
                contentHeight: imageFlow.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                Flow {
                    id: imageFlow
                    width: parent.width
                    spacing: Theme.spacingSm

                    Repeater {
                        model: imagePreviews

                        Rectangle {
                            id: thumbContainer
                            width: {
                                var cols = Math.max(2, Math.floor(imageFlow.width / 160))
                                return (imageFlow.width - (cols - 1) * Theme.spacingSm) / cols
                            }
                            height: width * 0.75
                            radius: Theme.radiusMedium
                            color: Theme.surface
                            border.color: modelData.previewUrl ? Theme.borderInfernal : Theme.error
                            border.width: 1
                            clip: true

                            Image {
                                anchors.fill: parent
                                anchors.margins: 2
                                source: modelData.previewUrl || ""
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                cache: false
                                visible: modelData.previewUrl ? true : false
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: !modelData.previewUrl
                                text: "missing"
                                color: Theme.error
                                font.family: Theme.monoFont
                                font.pixelSize: 11
                            }

                            Label {
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.margins: 3
                                text: "sealed"
                                color: Theme.terminalGreen
                                font.family: Theme.monoFont
                                font.pixelSize: 10
                                visible: !!modelData.previewUrl
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (modelData.previewUrl) {
                                        fullscreenViewer.attachmentId = modelData.attachmentId
                                        fullscreenViewer.noteId = modelData.noteId
                                        fullscreenViewer.imageSource = modelData.previewUrl
                                        fullscreenViewer.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }
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

    // Fullscreen image viewer popup (Part B)
    Popup {
        id: fullscreenViewer
        property string attachmentId: ""
        property int noteId: 0
        property string imageSource: ""

        anchors.centerIn: Overlay.overlay
        width: notesRoot.Window.width || notesRoot.width
        height: notesRoot.Window.height || notesRoot.height
        modal: true
        closePolicy: Popup.CloseOnEscape
        padding: 0

        background: Rectangle { color: "#e0000000" }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.spacingSm
                spacing: Theme.spacingSm

                GrimButton {
                    text: "Close"
                    onClicked: fullscreenViewer.close()
                }

                Item { Layout.fillWidth: true }

                GrimButton {
                    text: "Export"
                    primary: true
                    onClicked: {
                        exportAttachmentId = fullscreenViewer.attachmentId
                        exportNoteId = fullscreenViewer.noteId
                        exportFileDialog.open()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Image {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    source: fullscreenViewer.imageSource
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.margins: Theme.spacingSm
                horizontalAlignment: Text.AlignHCenter
                text: "Encrypted attachment • metadata purged"
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: 11
            }
        }
    }

    // Delete note confirmation
    Popup {
        id: deleteNotePopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(notesRoot.width - Theme.spacingLg * 2, 300)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.spacingMd

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.accent
            border.width: 1
            radius: Theme.radiusMedium
        }

        ColumnLayout {
            width: parent.width
            spacing: Theme.spacingMd

            Label {
                text: "Delete this note?"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "\"" + titleField.text + "\" will be permanently deleted."
                font.family: Theme.uiFont
                font.pixelSize: 13
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                GrimButton {
                    text: "Cancel"
                    Layout.fillWidth: true
                    onClicked: deleteNotePopup.close()
                }

                GrimButton {
                    text: "Delete"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        deleteNotePopup.close()
                        vault.deleteNote(currentNoteId)
                        clearNote()
                        refreshList()
                    }
                }
            }
        }
    }

    // State for export flow
    property string exportAttachmentId: ""
    property int exportNoteId: 0

    // SAF file picker for image import
    FileDialog {
        id: imageFileDialog
        title: "Insert image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)"]
        onAccepted: vault.insertImageIntoNote(currentNoteId, selectedFile)
    }

    // SAF save picker for export
    FileDialog {
        id: exportFileDialog
        title: "Export image"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG Image (*.png)"]
        onAccepted: {
            vault.exportAttachment(exportAttachmentId, exportNoteId, selectedFile)
        }
    }

    Connections {
        target: vault
        function onImageAttached(noteId, markdown, previewDataUrl) {
            if (noteId !== currentNoteId)
                return
            bodyField.insert(bodyField.cursorPosition, markdown)
            loadNotePreviews()
        }
        function onUnlockedChanged() {
            if (vault.unlocked && currentNoteId > 0)
                loadNotePreviews()
            else
                imagePreviews = []
        }
        function onAttachmentDeleted(attachmentId, noteId) {
            if (noteId === currentNoteId)
                loadNotePreviews()
        }
    }

    Component.onCompleted: noteList.model = vault.noteSummaries()
}
