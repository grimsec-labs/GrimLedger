import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: loginRoot
    signal unlocked()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 360)
        spacing: 16

        Label {
            text: "GRIMLEDGER"
            font.family: "monospace"
            font.pixelSize: 22
            color: vault.accentColor
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "> enter master key:"
            font.family: "monospace"
            color: "#33cc66"
        }

        TextField {
            id: passwordField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            placeholderText: "Master password"
            color: "#e8e8ec"
            background: Rectangle { color: "#111114"; border.color: "#331111"; radius: 3 }
        }

        Button {
            text: vault.vaultExists ? "Unlock Vault" : "Create Vault"
            Layout.fillWidth: true
            onClicked: {
                if (vault.vaultExists) {
                    if (vault.unlock(passwordField.text)) unlocked()
                } else {
                    if (vault.createVault(passwordField.text)) unlocked()
                }
            }
        }

        Button {
            text: "Biometric unlock"
            visible: vault.biometricSupported && vault.biometricConfigured
            Layout.fillWidth: true
            onClicked: { if (vault.biometricUnlock()) unlocked() }
        }

        Label {
            id: errorLabel
            color: "#ff4444"
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
