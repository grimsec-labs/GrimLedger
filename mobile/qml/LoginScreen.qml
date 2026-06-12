import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

Item {
    id: loginRoot
    signal unlocked()

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.loginGradientStart }
            GradientStop { position: 0.5; color: Theme.loginGradientMid }
            GradientStop { position: 1.0; color: Theme.loginGradientEnd }
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: formColumn.height + Theme.spacingXl * 2
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: formColumn
            width: Math.min(parent.width - Theme.spacingLg * 2, 360)
            x: (parent.width - width) / 2
            y: Math.max((loginRoot.height - height) / 2, Theme.spacingXl)
            spacing: Theme.spacingMd

            Label {
                text: "GRIMLEDGER"
                font.family: Theme.monoFont
                font.pixelSize: 22
                font.letterSpacing: 2
                color: Theme.accent
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "> enter master key:"
                font.family: Theme.monoFont
                font.pixelSize: 13
                color: Theme.terminalGreen
            }

            GrimTextField {
                id: passwordField
                Layout.fillWidth: true
                passwordMode: true
                monospace: true
                placeholderText: "Master password"
            }

            GrimButton {
                text: vault.vaultExists ? "Unlock Vault" : "Create Vault"
                primary: true
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
                visible: vault.biometricSupported && vault.biometricConfigured
                Layout.fillWidth: true
                onClicked: {
                    if (vault.biometricUnlock())
                        unlocked()
                }
            }

            Label {
                id: errorLabel
                color: Theme.error
                wrapMode: Text.WordWrap
                font.family: Theme.uiFont
                font.pixelSize: 13
                Layout.fillWidth: true
                visible: text.length > 0
            }
        }
    }

    Connections {
        target: vault
        function onErrorOccurred(message) { errorLabel.text = message }
    }
}
