import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grim

Item {
    id: loginRoot
    signal unlocked()

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#08080a" }
            GradientStop { position: 0.5; color: "#0e0a0c" }
            GradientStop { position: 1; color: "#0a0808" }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 360)
        spacing: Theme.spacing

        Image {
            source: "qrc:/logo.png"
            fillMode: Image.PreserveAspectFit
            Layout.preferredWidth: Math.min(parent.width, 320)
            Layout.preferredHeight: 140
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "// vault access"
            font.family: Theme.monoFont
            font.pixelSize: 11
            color: Theme.textDim
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Label {
                text: "> enter master key:"
                font.family: Theme.monoFont
                color: Theme.prompt
            }
            Label {
                id: blinkCursor
                text: "_"
                font.family: Theme.monoFont
                color: Theme.prompt
                Timer {
                    interval: 530
                    running: true
                    repeat: true
                    onTriggered: blinkCursor.visible = !blinkCursor.visible
                }
            }
        }

        GrimTextField {
            id: passwordField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            placeholderText: "Master password"
            onAccepted: actionButton.clicked()
        }

        GrimButton {
            id: actionButton
            text: vault.vaultExists ? "Unlock Vault" : "Create Vault"
            Layout.fillWidth: true
            onClicked: {
                if (vault.vaultExists) {
                    if (vault.unlock(passwordField.text))
                        unlocked()
                } else {
                    if (vault.createVault(passwordField.text))
                        unlocked()
                }
            }
        }

        GrimButton {
            text: "Biometric unlock"
            primary: false
            visible: vault.biometricSupported && vault.biometricConfigured
            Layout.fillWidth: true
            onClicked: { if (vault.biometricUnlock()) unlocked() }
        }

        Label {
            text: "⚠ Lost master passwords cannot be recovered."
            color: Theme.warning
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Label {
            id: errorLabel
            color: Theme.error
            font.family: Theme.monoFont
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            visible: text.length > 0
        }
    }

    Connections {
        target: vault
        function onErrorOccurred(message) { errorLabel.text = message }
    }
}
