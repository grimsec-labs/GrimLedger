import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

Item {
    id: credRoot

    property int currentCredId: 0
    property bool editing: false
    property string currentTotpCode: ""
    property int totpSeconds: 30

    function refreshList() {
        if (searchField.text.length > 0)
            credList.model = vault.searchCredentials(searchField.text)
        else
            credList.model = vault.credentialSummaries()
    }

    function loadCredential(id) {
        currentCredId = id
        var detail = vault.credentialDetail(id)
        labelField.text = detail.label || ""
        usernameField.text = detail.username || ""
        passwordField.text = detail.password || ""
        urlField.text = detail.url || ""
        notesField.text = detail.notes || ""
        totpField.text = detail.totpSecret || ""
        editing = true
        updateTotp()
    }

    function clearForm() {
        currentCredId = 0
        editing = false
        labelField.text = ""
        usernameField.text = ""
        passwordField.text = ""
        urlField.text = ""
        notesField.text = ""
        totpField.text = ""
        currentTotpCode = ""
    }

    function updateTotp() {
        if (currentCredId > 0) {
            currentTotpCode = vault.totpCode(currentCredId)
            totpSeconds = vault.totpSecondsRemaining()
        } else {
            currentTotpCode = ""
        }
    }

    Timer {
        interval: 1000
        running: currentTotpCode.length > 0
        repeat: true
        onTriggered: updateTotp()
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
                placeholderText: "Search credentials..."
                monospace: true
                onTextChanged: refreshList()
            }

            GrimButton {
                text: "+ New"
                onClicked: {
                    clearForm()
                    editing = true
                }
            }
        }

        Label {
            visible: credList.count === 0
            text: "> no vault keys"
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
                id: credList
                anchors.fill: parent
                anchors.margins: 1
                model: vault.credentialSummaries()
                clip: true
                spacing: 1

                delegate: GrimListItem {
                    width: ListView.view.width
                    text: modelData.label
                    subtitle: modelData.username + (modelData.url ? " · " + modelData.url : "")
                    subtitleMonospace: true
                    selected: currentCredId === modelData.id
                    onClicked: loadCredential(modelData.id)
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: formColumn.height
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            visible: editing

            ColumnLayout {
                id: formColumn
                width: parent.width
                spacing: Theme.spacingSm

                GrimTextField {
                    id: labelField
                    Layout.fillWidth: true
                    placeholderText: "Label"
                    monospace: true
                }

                GrimTextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Username"
                    monospace: true
                }

                GrimTextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Password"
                    passwordMode: true
                    monospace: true
                }

                GrimTextField {
                    id: urlField
                    Layout.fillWidth: true
                    placeholderText: "URL"
                    monospace: true
                }

                GrimTextField {
                    id: notesField
                    Layout.fillWidth: true
                    placeholderText: "Notes"
                }

                GrimTextField {
                    id: totpField
                    Layout.fillWidth: true
                    placeholderText: "TOTP secret (optional)"
                    monospace: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: currentTotpCode.length > 0
                    spacing: Theme.spacingSm

                    Label {
                        text: "TOTP: " + currentTotpCode
                        font.family: Theme.monoFont
                        font.pixelSize: 18
                        color: Theme.terminalGreen
                    }

                    Label {
                        text: totpSeconds + "s"
                        font.family: Theme.monoFont
                        font.pixelSize: 13
                        color: totpSeconds <= 5 ? Theme.error : Theme.textMuted
                    }

                    Item { Layout.fillWidth: true }

                    GrimButton {
                        text: "Copy"
                        onClicked: vault.copyToClipboard(currentTotpCode)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    GrimButton {
                        text: "Copy User"
                        enabled: usernameField.text.length > 0
                        onClicked: vault.copyToClipboard(usernameField.text)
                    }

                    GrimButton {
                        text: "Copy Pass"
                        enabled: passwordField.text.length > 0
                        onClicked: vault.copyToClipboard(passwordField.text)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm
                    Layout.topMargin: Theme.spacingSm

                    GrimButton {
                        text: "Delete"
                        visible: currentCredId > 0
                        onClicked: deleteCredPopup.open()
                    }

                    Item { Layout.fillWidth: true }

                    GrimButton {
                        text: "Cancel"
                        onClicked: clearForm()
                    }

                    GrimButton {
                        text: "Save"
                        primary: true
                        onClicked: {
                            if (currentCredId > 0) {
                                vault.updateCredential(currentCredId, labelField.text,
                                    usernameField.text, passwordField.text,
                                    urlField.text, notesField.text, totpField.text)
                            } else {
                                var newId = vault.createCredential(labelField.text,
                                    usernameField.text, passwordField.text,
                                    urlField.text, notesField.text, totpField.text)
                                if (newId > 0) currentCredId = newId
                            }
                            refreshList()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: deleteCredPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(credRoot.width - Theme.spacingLg * 2, 300)
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
                text: "Delete this credential?"
                font.family: Theme.monoFont
                font.pixelSize: 15
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "\"" + labelField.text + "\" will be permanently deleted."
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
                    onClicked: deleteCredPopup.close()
                }

                GrimButton {
                    text: "Delete"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        deleteCredPopup.close()
                        vault.deleteCredential(currentCredId)
                        clearForm()
                        refreshList()
                    }
                }
            }
        }
    }

    Component.onCompleted: credList.model = vault.credentialSummaries()
}
