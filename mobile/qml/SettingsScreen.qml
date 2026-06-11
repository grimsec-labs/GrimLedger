import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

ScrollView {
    id: settingsRoot
    anchors.fill: parent
    clip: true

    ColumnLayout {
        width: settingsRoot.width - Theme.spacingMd * 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: Theme.spacingMd
        spacing: Theme.spacingMd

        GrimSectionLabel {
            text: "Appearance"
            Layout.fillWidth: true
        }

        Label {
            text: "Accent color"
            color: Theme.textMuted
            font.family: Theme.uiFont
            font.pixelSize: 13
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Repeater {
                model: Theme.accentPresets
                delegate: Rectangle {
                    Layout.preferredWidth: Theme.touchTarget
                    Layout.preferredHeight: 36
                    radius: Theme.radiusSmall
                    color: modelData.hex
                    border.color: vault.accentColor === modelData.hex ? Theme.textBright : Theme.borderInfernal
                    border.width: vault.accentColor === modelData.hex ? 2 : 1

                    TapHandler {
                        onTapped: vault.accentColor = modelData.hex
                    }
                }
            }
        }

        GrimSectionLabel {
            text: "Editor"
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSm
        }

        GrimCheckBox {
            id: lineNumbersCheck
            text: "Show line numbers"
            checked: vault.lineNumbers()
        }

        GrimCheckBox {
            id: wordWrapCheck
            text: "Word wrap in editor"
            checked: vault.wordWrap()
        }

        GrimCheckBox {
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

        GrimSectionLabel {
            text: "Biometric unlock"
            visible: vault.biometricSupported
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSm
        }

        RowLayout {
            Layout.fillWidth: true
            visible: vault.biometricSupported
            spacing: Theme.spacingSm

            GrimTextField {
                id: biometricPasswordField
                visible: vault.biometricSupported && !vault.biometricConfigured
                passwordMode: true
                Layout.fillWidth: true
                placeholderText: "Password to enable biometric"
            }

            GrimButton {
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
            spacing: Theme.spacingSm
            Layout.topMargin: Theme.spacingSm

            GrimButton {
                text: "Reset to Defaults"
                onClicked: {
                    vault.resetSettings()
                    lineNumbersCheck.checked = vault.lineNumbers()
                    wordWrapCheck.checked = vault.wordWrap()
                }
            }

            Item { Layout.fillWidth: true }

            GrimButton {
                text: "Save Settings"
                primary: true
                onClicked: vault.saveSettings(
                    lineNumbersCheck.checked,
                    wordWrapCheck.checked,
                    autoLockCheck.checked,
                    autoLockSpin.value)
            }
        }

        GrimButton {
            text: "Lock Vault"
            primary: true
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSm
            onClicked: vault.lock()
        }
    }
}
