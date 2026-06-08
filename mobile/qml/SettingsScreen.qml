import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grim

ScrollView {
    anchors.fill: parent

    ColumnLayout {
        width: parent.width
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        Label {
            text: "Vault Settings"
            color: Theme.accent()
            font.pixelSize: 18
            font.bold: true
            font.family: Theme.monoFont
        }

        CheckBox {
            id: lineNumbersCheck
            text: "Show line numbers"
            checked: vault.lineNumbers()
            font.family: Theme.monoFont
        }
        CheckBox {
            id: wordWrapCheck
            text: "Word wrap in editor"
            checked: vault.wordWrap()
            font.family: Theme.monoFont
        }
        CheckBox {
            id: autoLockCheck
            text: "Auto-lock after inactivity"
            checked: true
            font.family: Theme.monoFont
        }
        SpinBox {
            id: autoLockSpin
            from: 1
            to: 120
            value: 15
        }

        Label {
            text: "Biometric unlock"
            color: Theme.accent()
            font.bold: true
            font.family: Theme.monoFont
            visible: vault.biometricSupported
        }
        RowLayout {
            Layout.fillWidth: true
            visible: vault.biometricSupported
            spacing: 8
            GrimTextField {
                id: biometricPasswordField
                visible: vault.biometricSupported && !vault.biometricConfigured
                echoMode: TextInput.Password
                placeholderText: "Password to enable biometric"
                Layout.fillWidth: true
            }
            GrimButton {
                text: vault.biometricConfigured ? "Disable biometric" : "Enable biometric"
                primary: false
                onClicked: {
                    if (vault.biometricConfigured) {
                        vault.disableBiometric()
                    } else {
                        vault.enableBiometric(biometricPasswordField.text)
                        biometricPasswordField.text = ""
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            GrimButton {
                text: "Reset to Defaults"
                primary: false
                onClicked: {
                    vault.resetSettings()
                    lineNumbersCheck.checked = vault.lineNumbers()
                    wordWrapCheck.checked = vault.wordWrap()
                }
            }
            GrimButton {
                text: "Save Settings"
                primary: false
                onClicked: vault.saveSettings(
                    lineNumbersCheck.checked,
                    wordWrapCheck.checked,
                    autoLockCheck.checked,
                    autoLockSpin.value)
            }
        }

        GrimButton {
            text: "Lock Vault"
            Layout.fillWidth: true
            onClicked: vault.lock()
        }

        Label {
            text: "Android uses a separate vault file from desktop. Sync via GrimShare when available."
            color: Theme.textDim
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.family: Theme.monoFont
        }
    }
}
