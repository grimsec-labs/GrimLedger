import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grim

Item {
    id: credRoot

    property int currentCredId: 0
    property bool editing: false

    function clearEditor() {
        currentCredId = 0
        editing = false
        labelField.text = ""
        usernameField.text = ""
        passwordField.text = ""
        urlField.text = ""
    }

    function loadCredential(id) {
        const details = vault.credentialDetails(id)
        if (!details.id)
            return
        currentCredId = details.id
        editing = true
        labelField.text = details.label
        usernameField.text = details.username
        passwordField.text = details.password
        urlField.text = details.url
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            GrimButton {
                text: "+ New Key"
                onClicked: clearEditor()
            }
            GrimButton {
                text: editing ? "Save" : "Add"
                primary: false
                enabled: labelField.text.length > 0 || usernameField.text.length > 0
                onClicked: {
                    if (editing && currentCredId > 0) {
                        vault.updateCredential(
                            currentCredId,
                            labelField.text,
                            usernameField.text,
                            passwordField.text,
                            urlField.text)
                    } else {
                        const id = vault.createCredential(
                            labelField.text,
                            usernameField.text,
                            passwordField.text,
                            urlField.text)
                        if (id > 0) {
                            currentCredId = id
                            editing = true
                        }
                    }
                }
            }
            GrimButton {
                text: "Delete"
                primary: false
                visible: editing && currentCredId > 0
                onClicked: {
                    vault.deleteCredential(currentCredId)
                    clearEditor()
                }
            }
        }

        GrimTextField {
            id: labelField
            Layout.fillWidth: true
            placeholderText: "Label"
        }
        GrimTextField {
            id: usernameField
            Layout.fillWidth: true
            placeholderText: "Username"
        }
        GrimTextField {
            id: passwordField
            Layout.fillWidth: true
            placeholderText: "Password"
            echoMode: TextInput.Password
        }
        GrimTextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "URL"
            inputMethodHints: Qt.ImhUrlCharactersOnly
        }

        Label {
            text: vault.credentials.length === 0
                ? "No credentials yet. Add a key above, or import a vault from desktop."
            : "Saved keys"
            color: Theme.textMuted
            font.family: Theme.monoFont
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        ListView {
            id: credList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: vault.credentials
            clip: true
            spacing: 6
            delegate: GrimFrame {
                width: ListView.view.width
                required property var modelData
                highlighted: currentCredId === modelData.id

                MouseArea {
                    anchors.fill: parent
                    onClicked: loadCredential(modelData.id)
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 4
                    Label {
                        text: modelData.label
                        color: Theme.accent()
                        font.bold: true
                        font.family: Theme.monoFont
                    }
                    Label {
                        text: modelData.username
                        color: Theme.text
                        font.family: Theme.monoFont
                    }
                    Label {
                        text: modelData.url
                        color: Theme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        font.family: Theme.monoFont
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
