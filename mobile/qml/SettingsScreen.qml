import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    anchors.fill: parent
    ColumnLayout {
        width: parent.width
        anchors.margins: 16
        spacing: 12

        Label { text: "Vault Settings"; color: "#cc2200"; font.pixelSize: 18; font.bold: true }

        CheckBox {
            id: lineNumbersCheck
            text: "Show line numbers"
            checked: vault.lineNumbers()
        }
        CheckBox {
            id: wordWrapCheck
            text: "Word wrap in editor"
            checked: vault.wordWrap()
        }
        CheckBox {
            id: autoLockCheck
            text: "Auto-lock after inactivity"
            checked: true
        }
        SpinBox {
            id: autoLockSpin
            from: 1
            to: 120
            value: 15
        }

        Label {
            text: "Biometric unlock"
            color: "#cc2200"
            font.bold: true
            visible: vault.biometricSupported
        }
        RowLayout {
            Layout.fillWidth: true
            visible: vault.biometricSupported
            spacing: 8
            TextField {
                id: biometricPasswordField
                visible: vault.biometricSupported && !vault.biometricConfigured
                echoMode: TextInput.Password
                placeholderText: "Password to enable biometric"
                Layout.fillWidth: true
                color: "#e8e8ec"
                background: Rectangle { color: "#111114"; border.color: "#331111"; radius: 3 }
            }
            Button {
                text: vault.biometricConfigured ? "Disable biometric" : "Enable biometric"
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
            Button {
                text: "Reset to Defaults"
                onClicked: {
                    vault.resetSettings()
                    lineNumbersCheck.checked = vault.lineNumbers()
                    wordWrapCheck.checked = vault.wordWrap()
                }
            }
            Button {
                text: "Save Settings"
                onClicked: vault.saveSettings(
                    lineNumbersCheck.checked,
                    wordWrapCheck.checked,
                    autoLockCheck.checked,
                    autoLockSpin.value)
            }
        }

        Button {
            text: "Lock Vault"
            Layout.fillWidth: true
            onClicked: vault.lock()
        }
    }
}
