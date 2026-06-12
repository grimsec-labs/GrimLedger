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
    property int filterFolderId: 0
    property bool filterFavoritesOnly: false
    property int sortField: 2
    property bool sortDescending: true
    property int noteFolderId: 0
    property bool noteIsFavorite: false
    property var noteTags: []

    // Multi-select state
    property bool selectionMode: false
    property var selectedIds: ({})
    property int selectedCount: 0

    function refreshList() {
        if (searchField.text.length > 0)
            noteList.model = vault.searchNotes(searchField.text)
        else
            noteList.model = vault.noteSummariesFiltered(filterFolderId, filterFavoritesOnly, sortField, sortDescending)
        pruneSelection()
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
        var detail = vault.noteDetail(noteId)
        noteFolderId = detail.folderId || 0
        noteIsFavorite = detail.isFavorite || false
        noteTags = detail.tags || []
        tagsField.text = noteTags.join(", ")
        loadNotePreviews()
    }

    function clearNote() {
        currentNoteId = 0
        titleField.text = ""
        bodyField.text = ""
        imagePreviews = []
        noteFolderId = 0
        noteIsFavorite = false
        noteTags = []
        tagsField.text = ""
    }

    function enterSelectionMode(noteId) {
        selectionMode = true
        selectedIds = {}
        toggleSelection(noteId)
    }

    function exitSelectionMode() {
        selectionMode = false
        selectedIds = {}
        selectedCount = 0
    }

    function toggleSelection(noteId) {
        var s = selectedIds
        if (s[noteId]) {
            delete s[noteId]
        } else {
            s[noteId] = true
        }
        selectedIds = s
        var count = 0
        for (var k in selectedIds) { if (selectedIds[k]) count++ }
        selectedCount = count
        if (selectedCount === 0)
            exitSelectionMode()
    }

    function isSelected(noteId) {
        return selectedIds[noteId] === true
    }

    function getSelectedIdList() {
        var list = []
        for (var k in selectedIds) {
            if (selectedIds[k]) list.push(parseInt(k))
        }
        return list
    }

    function pruneSelection() {
        if (!selectionMode) return
        var visibleIds = {}
        for (var i = 0; i < noteList.count; i++) {
            var item = noteList.model[i]
            if (item) visibleIds[item.id] = true
        }
        var s = selectedIds
        var changed = false
        for (var k in s) {
            if (!visibleIds[k]) {
                delete s[k]
                changed = true
            }
        }
        if (changed) {
            selectedIds = s
            var count = 0
            for (var k2 in selectedIds) { if (selectedIds[k2]) count++ }
            selectedCount = count
            if (selectedCount === 0) exitSelectionMode()
        }
    }

    function refreshFolderFilter() {
        var items = [{"text": "All Folders", "value": 0}]
        var flds = vault.unlocked ? vault.folders() : []
        for (var i = 0; i < flds.length; i++)
            items.push({"text": flds[i].name, "value": flds[i].id})
        folderFilter.model = items
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
                visible: !selectionMode
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

            ComboBox {
                id: folderFilter
                Layout.fillWidth: true
                model: {
                    var items = [{"text": "All Folders", "value": 0}]
                    var flds = vault.unlocked ? vault.folders() : []
                    for (var i = 0; i < flds.length; i++)
                        items.push({"text": flds[i].name, "value": flds[i].id})
                    return items
                }
                textRole: "text"
                valueRole: "value"
                currentIndex: 0
                onActivated: {
                    filterFolderId = currentValue
                    refreshList()
                }
                font.family: Theme.monoFont
                font.pixelSize: 12
            }

            GrimButton {
                text: filterFavoritesOnly ? "Fav ON" : "Fav"
                onClicked: {
                    filterFavoritesOnly = !filterFavoritesOnly
                    refreshList()
                }
            }

            ComboBox {
                id: sortCombo
                Layout.preferredWidth: 140
                model: [
                    {"text": "Modified ↓", "field": 2, "desc": true},
                    {"text": "Created ↓",  "field": 1, "desc": true},
                    {"text": "Title A-Z",  "field": 0, "desc": false},
                    {"text": "Title Z-A",  "field": 0, "desc": true}
                ]
                textRole: "text"
                currentIndex: 0
                onActivated: {
                    var item = model[currentIndex]
                    sortField = item.field
                    sortDescending = item.desc
                    refreshList()
                }
                font.family: Theme.monoFont
                font.pixelSize: 12
            }

            GrimButton {
                text: "Folders"
                onClicked: folderManagementPopup.open()
            }
        }

        // Selection mode action bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm
            visible: selectionMode

            Label {
                text: selectedCount + " selected"
                color: Theme.accent
                font.family: Theme.monoFont
                font.pixelSize: 13
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            GrimButton {
                text: "Fav"
                enabled: selectedCount > 0
                onClicked: {
                    vault.setNotesFavorite(getSelectedIdList(), true)
                    exitSelectionMode()
                    refreshList()
                }
            }

            GrimButton {
                text: "Move"
                enabled: selectedCount > 0
                onClicked: moveToFolderPopup.open()
            }

            GrimButton {
                text: "Export"
                enabled: selectedCount > 0
                onClicked: {
                    exportTargetNoteId = 0
                    exportWarningPopup.open()
                }
            }

            GrimButton {
                text: "Delete"
                enabled: selectedCount > 0
                onClicked: bulkDeletePopup.open()
            }

            GrimButton {
                text: "Cancel"
                onClicked: exitSelectionMode()
            }
        }

        // Normal action bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm
            visible: !selectionMode

            GrimButton {
                text: "Delete"
                enabled: currentNoteId > 0
                onClicked: deleteNotePopup.open()
            }

            GrimButton {
                text: "Export"
                enabled: currentNoteId > 0
                onClicked: {
                    exportTargetNoteId = currentNoteId
                    exportWarningPopup.open()
                }
            }

            GrimButton {
                text: "Image"
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
                    var tagList = tagsField.text.split(",").map(function(t) { return t.trim() }).filter(function(t) { return t.length > 0 })
                    vault.saveNoteEx(currentNoteId, titleField.text, bodyField.text,
                                     noteFolderId, noteIsFavorite, tagList)
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
            border.color: selectionMode ? Theme.accent : Theme.border
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
                    selected: !selectionMode && currentNoteId === modelData.id
                    multiSelected: selectionMode && isSelected(modelData.id)
                    onClicked: {
                        if (selectionMode) {
                            toggleSelection(modelData.id)
                        } else {
                            openNote(modelData.id, modelData.title)
                        }
                    }
                    onPressAndHold: {
                        if (!selectionMode)
                            enterSelectionMode(modelData.id)
                    }
                }
            }

            Label {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                visible: selectionMode
                text: "long-press to select"
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: 10
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm
            visible: !selectionMode

            GrimTextField {
                id: titleField
                Layout.fillWidth: true
                placeholderText: "Title"
                monospace: true
            }

            GrimButton {
                text: noteIsFavorite ? "Unfav" : "Fav"
                enabled: currentNoteId > 0
                onClicked: noteIsFavorite = !noteIsFavorite
            }
        }

        GrimTextField {
            id: tagsField
            Layout.fillWidth: true
            placeholderText: "Tags (comma-separated)"
            monospace: true
            font.pixelSize: 12
            visible: !selectionMode
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: imageGrid.contentHeight + Theme.spacingSm * 2
            Layout.maximumHeight: 280
            visible: imagePreviews.length > 0 && !selectionMode
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

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.borderInfernal
            border.width: 1
            radius: Theme.radiusSmall
            clip: true
            visible: !selectionMode

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    id: lineNumberGutter
                    visible: vault.lineNumbers()
                    Layout.fillHeight: true
                    Layout.preferredWidth: 36
                    color: Theme.listBackground

                    Flickable {
                        anchors.fill: parent
                        contentY: bodyFlickable.contentY
                        interactive: false
                        clip: true

                        Column {
                            width: parent.width
                            topPadding: bodyField.topPadding

                            Repeater {
                                model: {
                                    var text = bodyField.text || ""
                                    var count = text.split("\n").length
                                    return Math.max(count, 1)
                                }

                                Label {
                                    width: lineNumberGutter.width
                                    height: bodyField.font.pixelSize * 1.45
                                    text: (index + 1)
                                    font.family: Theme.monoFont
                                    font.pixelSize: bodyField.font.pixelSize - 2
                                    color: Theme.textDim
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignTop
                                    rightPadding: Theme.spacingXs
                                }
                            }
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: Theme.borderInfernal
                    }
                }

                Flickable {
                    id: bodyFlickable
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: bodyField.useWordWrap ? width : bodyField.implicitWidth
                    contentHeight: bodyField.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    flickableDirection: bodyField.useWordWrap ? Flickable.VerticalFlick : Flickable.HorizontalAndVerticalFlick

                    GrimTextArea {
                        id: bodyField
                        width: useWordWrap ? bodyFlickable.width : Math.max(implicitWidth, bodyFlickable.width)
                        useWordWrap: vault.wordWrap()
                        background: null
                    }
                }
            }
        }
    }

    // Fullscreen image viewer popup
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

    // Delete single note confirmation
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

    // Bulk delete confirmation
    Popup {
        id: bulkDeletePopup
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
                text: "Delete " + selectedCount + " note(s)?"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "Selected notes will be permanently deleted."
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
                    onClicked: bulkDeletePopup.close()
                }

                GrimButton {
                    text: "Delete All"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        bulkDeletePopup.close()
                        var ids = getSelectedIdList()
                        vault.deleteNotes(ids)
                        if (ids.indexOf(currentNoteId) >= 0)
                            clearNote()
                        exitSelectionMode()
                        refreshList()
                    }
                }
            }
        }
    }

    // Export warning popup
    property int exportTargetNoteId: 0

    Popup {
        id: exportWarningPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(notesRoot.width - Theme.spacingLg * 2, 340)
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
                text: "Export Warning"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "Exported notes are plaintext files outside the encrypted GrimLedger vault. Anyone with access to the exported files may read them."
                font.family: Theme.uiFont
                font.pixelSize: 13
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: "Image attachments (grim:// references) are not included in the export."
                font.family: Theme.uiFont
                font.pixelSize: 11
                color: Theme.textDim
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                GrimButton {
                    text: "Cancel"
                    Layout.fillWidth: true
                    onClicked: exportWarningPopup.close()
                }

                GrimButton {
                    text: "Markdown"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        exportWarningPopup.close()
                        exportMdDialog.open()
                    }
                }

                GrimButton {
                    text: "HTML"
                    Layout.fillWidth: true
                    enabled: exportTargetNoteId > 0
                    onClicked: {
                        exportWarningPopup.close()
                        exportHtmlDialog.open()
                    }
                }
            }
        }
    }

    // Camera permission explanation popup
    Popup {
        id: cameraPermPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(notesRoot.width - Theme.spacingLg * 2, 320)
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
                text: "Camera Permission"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "GrimLedger needs camera access to take photos for your encrypted vault. Photos are sanitized to remove metadata before encrypted storage."
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
                    text: "Not now"
                    Layout.fillWidth: true
                    onClicked: cameraPermPopup.close()
                }

                GrimButton {
                    text: "Allow camera"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        cameraPermPopup.close()
                        vault.requestCameraPermission()
                    }
                }
            }
        }
    }

    // Camera permission denied popup
    Popup {
        id: cameraDeniedPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(notesRoot.width - Theme.spacingLg * 2, 320)
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
                text: "Camera Disabled"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "Camera permission is disabled. To take encrypted vault photos, enable Camera permission for GrimLedger in Android Settings."
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
                    onClicked: cameraDeniedPopup.close()
                }

                GrimButton {
                    text: "Open Settings"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        cameraDeniedPopup.close()
                        vault.openAppSettings()
                    }
                }
            }
        }
    }

    // Move to folder popup
    Popup {
        id: moveToFolderPopup
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
                text: "Move " + selectedCount + " note(s) to folder"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Repeater {
                model: {
                    var items = [{"text": "No Folder", "value": 0}]
                    var flds = vault.unlocked ? vault.folders() : []
                    for (var i = 0; i < flds.length; i++)
                        items.push({"text": flds[i].name, "value": flds[i].id})
                    return items
                }

                GrimButton {
                    text: modelData.text
                    Layout.fillWidth: true
                    onClicked: {
                        moveToFolderPopup.close()
                        vault.moveNotesToFolder(getSelectedIdList(), modelData.value)
                        exitSelectionMode()
                        refreshList()
                    }
                }
            }

            GrimButton {
                text: "Cancel"
                Layout.fillWidth: true
                onClicked: moveToFolderPopup.close()
            }
        }
    }

    // Folder management popup
    Popup {
        id: folderManagementPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(notesRoot.width - Theme.spacingLg * 2, 340)
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.spacingMd

        property var folderList: []
        property int editingFolderId: 0

        onOpened: {
            folderList = vault.unlocked ? vault.folders() : []
            newFolderField.text = ""
            editingFolderId = 0
        }

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
                text: "Manage Folders"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                GrimTextField {
                    id: newFolderField
                    Layout.fillWidth: true
                    placeholderText: "New folder name"
                    monospace: true
                    font.pixelSize: 13
                }

                GrimButton {
                    text: "Add"
                    enabled: newFolderField.text.trim().length > 0
                    onClicked: {
                        vault.createFolder(newFolderField.text.trim())
                        newFolderField.text = ""
                        folderManagementPopup.folderList = vault.folders()
                        refreshFolderFilter()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(folderListView.contentHeight + 4, 200)
                color: Theme.listBackground
                border.color: Theme.border
                border.width: 1
                radius: Theme.radiusSmall
                visible: folderManagementPopup.folderList.length > 0

                ListView {
                    id: folderListView
                    anchors.fill: parent
                    anchors.margins: 2
                    model: folderManagementPopup.folderList
                    clip: true
                    spacing: 1

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 40
                        color: "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingSm
                            anchors.rightMargin: Theme.spacingSm
                            spacing: Theme.spacingSm

                            GrimTextField {
                                id: folderNameField
                                Layout.fillWidth: true
                                text: modelData.name
                                monospace: true
                                font.pixelSize: 13
                                readOnly: folderManagementPopup.editingFolderId !== modelData.id
                                onEditingFinished: {
                                    if (text.trim().length > 0 && text.trim() !== modelData.name) {
                                        vault.renameFolder(modelData.id, text.trim())
                                        folderManagementPopup.folderList = vault.folders()
                                        refreshFolderFilter()
                                    }
                                    folderManagementPopup.editingFolderId = 0
                                }
                            }

                            GrimButton {
                                text: folderManagementPopup.editingFolderId === modelData.id ? "Done" : "Edit"
                                onClicked: {
                                    if (folderManagementPopup.editingFolderId === modelData.id) {
                                        folderNameField.editingFinished()
                                    } else {
                                        folderManagementPopup.editingFolderId = modelData.id
                                        folderNameField.forceActiveFocus()
                                    }
                                }
                            }

                            GrimButton {
                                text: "Del"
                                onClicked: {
                                    folderDeleteConfirm.folderId = modelData.id
                                    folderDeleteConfirm.folderName = modelData.name
                                    folderDeleteConfirm.open()
                                }
                            }
                        }
                    }
                }
            }

            Label {
                visible: folderManagementPopup.folderList.length === 0
                text: "No folders created yet."
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: 12
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            GrimButton {
                text: "Close"
                Layout.fillWidth: true
                onClicked: folderManagementPopup.close()
            }
        }
    }

    // Folder delete confirmation
    Popup {
        id: folderDeleteConfirm
        property int folderId: 0
        property string folderName: ""

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
                text: "Delete folder?"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "\"" + folderDeleteConfirm.folderName + "\" will be deleted. Notes in this folder will be moved to No Folder."
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
                    onClicked: folderDeleteConfirm.close()
                }

                GrimButton {
                    text: "Delete"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        folderDeleteConfirm.close()
                        vault.deleteFolderSafe(folderDeleteConfirm.folderId)
                        folderManagementPopup.folderList = vault.folders()
                        refreshFolderFilter()
                        refreshList()
                    }
                }
            }
        }
    }

    // State for image export flow
    property string exportAttachmentId: ""
    property int exportNoteId: 0

    // SAF file picker for image import
    FileDialog {
        id: imageFileDialog
        title: "Insert image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)"]
        onAccepted: vault.insertImageIntoNote(currentNoteId, selectedFile)
    }

    // SAF save picker for image export
    FileDialog {
        id: exportFileDialog
        title: "Export image"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG Image (*.png)"]
        onAccepted: {
            vault.exportAttachment(exportAttachmentId, exportNoteId, selectedFile)
        }
    }

    // SAF save picker for note markdown export
    FileDialog {
        id: exportMdDialog
        title: "Export as Markdown"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Markdown (*.md)"]
        onAccepted: {
            if (exportTargetNoteId > 0) {
                vault.exportNoteMarkdown(exportTargetNoteId, selectedFile)
            } else if (selectionMode && selectedCount > 0) {
                vault.exportNotesMarkdown(getSelectedIdList(), selectedFile)
                exitSelectionMode()
            }
        }
    }

    // SAF save picker for note HTML export
    FileDialog {
        id: exportHtmlDialog
        title: "Export as HTML"
        fileMode: FileDialog.SaveFile
        nameFilters: ["HTML (*.html)"]
        onAccepted: {
            if (exportTargetNoteId > 0) {
                vault.exportNoteHtml(exportTargetNoteId, selectedFile)
            }
        }
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: {
            if (!vault.unlocked) return
            currentNoteId = vault.createNote("Untitled")
            refreshList()
            titleField.text = "Untitled"
            bodyField.text = ""
            imagePreviews = []
        }
    }
    Shortcut {
        sequence: "Ctrl+S"
        onActivated: {
            if (currentNoteId <= 0) return
            var tagList = tagsField.text.split(",").map(function(t) { return t.trim() }).filter(function(t) { return t.length > 0 })
            vault.saveNoteEx(currentNoteId, titleField.text, bodyField.text,
                             noteFolderId, noteIsFavorite, tagList)
            refreshList()
            loadNotePreviews()
        }
    }
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: searchField.forceActiveFocus()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (selectionMode) exitSelectionMode()
        }
    }
    Shortcut {
        sequence: "Delete"
        onActivated: {
            if (selectionMode && selectedCount > 0)
                bulkDeletePopup.open()
            else if (currentNoteId > 0)
                deleteNotePopup.open()
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
            if (vault.unlocked && currentNoteId > 0) {
                bodyField.text = vault.noteBody(currentNoteId)
                loadNotePreviews()
            } else {
                imagePreviews = []
            }
        }
        function onAttachmentDeleted(attachmentId, noteId) {
            if (noteId === currentNoteId)
                loadNotePreviews()
        }
        function onCameraPermissionNeeded() {
            cameraPermPopup.open()
        }
        function onCameraPermissionResult(granted) {
            if (!granted)
                cameraDeniedPopup.open()
        }
    }

    Component.onCompleted: noteList.model = vault.noteSummaries()
}
