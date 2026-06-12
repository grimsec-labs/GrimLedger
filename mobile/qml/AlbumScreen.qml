import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Theme 1.0
import controls 1.0

Item {
    id: albumRoot

    property var albumItems: []
    property bool loading: false

    function refresh() {
        if (!vault.unlocked) {
            albumItems = []
            return
        }
        loading = true
        albumItems = vault.allImageAttachments()
        loading = false
    }

    Connections {
        target: vault
        function onUnlockedChanged() {
            if (vault.unlocked)
                refresh()
            else
                albumItems = []
        }
        function onImageAttached() { refresh() }
        function onAttachmentDeleted() { refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        spacing: Theme.spacingSm

        Label {
            text: "Purged Images"
            font.family: Theme.monoFont
            font.pixelSize: 17
            font.bold: true
            color: Theme.accent
            Layout.fillWidth: true
        }

        Label {
            text: "Encrypted, metadata-stripped images in your vault"
            font.family: Theme.monoFont
            font.pixelSize: 11
            color: Theme.textDim
            Layout.fillWidth: true
        }

        Label {
            visible: !vault.unlocked
            text: "> vault locked — unlock to view images"
            color: Theme.textDim
            font.family: Theme.monoFont
            font.pixelSize: 13
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            visible: vault.unlocked && albumItems.length === 0 && !loading
            text: "> no images yet — insert one from a note"
            color: Theme.textDim
            font.family: Theme.monoFont
            font.pixelSize: 13
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: albumFlow.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            visible: vault.unlocked && albumItems.length > 0

            Flow {
                id: albumFlow
                width: parent.width
                spacing: Theme.spacingSm

                Repeater {
                    model: albumItems

                    Rectangle {
                        width: {
                            var cols = Math.max(2, Math.floor(albumFlow.width / 170))
                            return (albumFlow.width - (cols - 1) * Theme.spacingSm) / cols
                        }
                        height: width
                        radius: Theme.radiusMedium
                        color: Theme.surface
                        border.color: Theme.borderInfernal
                        border.width: 1
                        clip: true

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            source: modelData.previewUrl || ""
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            cache: false
                            visible: !!modelData.previewUrl
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: !modelData.previewUrl
                            text: "decrypt failed"
                            color: Theme.error
                            font.family: Theme.monoFont
                            font.pixelSize: 10
                        }

                        // Overlay with note title + date
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: metaCol.implicitHeight + Theme.spacingXs * 2
                            color: "#c0000000"
                            radius: Theme.radiusMedium

                            ColumnLayout {
                                id: metaCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.margins: Theme.spacingXs
                                spacing: 1

                                Label {
                                    visible: !!modelData.noteTitle
                                    text: modelData.noteTitle || ""
                                    font.family: Theme.monoFont
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: {
                                        if (!modelData.createdAt) return ""
                                        var d = new Date(modelData.createdAt * 1000)
                                        return Qt.formatDateTime(d, "yyyy-MM-dd HH:mm")
                                    }
                                    font.family: Theme.monoFont
                                    font.pixelSize: 9
                                    color: Theme.textDim
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                albumPreview.attachmentId = modelData.id
                                albumPreview.noteId = modelData.noteId
                                albumPreview.imageSource = modelData.previewUrl || ""
                                albumPreview.noteTitle = modelData.noteTitle || ""
                                albumPreview.open()
                            }
                        }
                    }
                }
            }
        }
    }

    // Fullscreen preview for album
    Popup {
        id: albumPreview
        property string attachmentId: ""
        property int noteId: 0
        property string imageSource: ""
        property string noteTitle: ""

        anchors.centerIn: Overlay.overlay
        width: albumRoot.Window.width || albumRoot.width
        height: albumRoot.Window.height || albumRoot.height
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
                    onClicked: albumPreview.close()
                }

                Item { Layout.fillWidth: true }

                GrimButton {
                    text: "Export"
                    onClicked: {
                        albumExportId = albumPreview.attachmentId
                        albumExportNoteId = albumPreview.noteId
                        albumExportDialog.open()
                    }
                }

                GrimButton {
                    text: "Delete"
                    primary: true
                    onClicked: albumDeletePopup.open()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Image {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    source: albumPreview.imageSource
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.margins: Theme.spacingSm
                horizontalAlignment: Text.AlignHCenter
                text: albumPreview.noteTitle
                      ? ("Linked to: " + albumPreview.noteTitle)
                      : "Encrypted attachment"
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: 11
            }

            Label {
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.spacingSm
                horizontalAlignment: Text.AlignHCenter
                text: "Metadata purged • XChaCha20 encrypted"
                color: Theme.textDim
                font.family: Theme.monoFont
                font.pixelSize: 10
            }
        }
    }

    property string albumExportId: ""
    property int albumExportNoteId: 0

    FileDialog {
        id: albumExportDialog
        title: "Export image"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG Image (*.png)"]
        onAccepted: {
            vault.exportAttachment(albumExportId, albumExportNoteId, selectedFile)
        }
    }

    // Delete confirmation
    Popup {
        id: albumDeletePopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(albumRoot.width - Theme.spacingLg * 2, 300)
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
                text: "Delete this image?"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "This encrypted image will be permanently removed from the vault."
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
                    onClicked: albumDeletePopup.close()
                }

                GrimButton {
                    text: "Delete"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        albumDeletePopup.close()
                        vault.deleteAttachment(albumPreview.attachmentId, albumPreview.noteId)
                        albumPreview.close()
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (vault.unlocked)
            refresh()
    }
}
