import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

Item {
    id: settingsRoot

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: settingsColumn.height + Theme.spacingMd * 2
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: settingsColumn
            width: parent.width - Theme.spacingMd * 2
            x: Theme.spacingMd
            y: Theme.spacingMd
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
                    visible: !vault.biometricConfigured
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
                            if (biometricPasswordField.text.length === 0) {
                                vault.errorOccurred("Enter your master password to enable biometric unlock.")
                                return
                            }
                            biometricConfirmPopup.open()
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

    Popup {
        id: biometricConfirmPopup
        anchors.centerIn: Overlay.overlay
        width: Math.min(settingsRoot.width - Theme.spacingLg * 2, 340)
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
                text: "Enable biometric unlock?"
                font.family: Theme.monoFont
                font.pixelSize: 16
                color: Theme.accent
                Layout.fillWidth: true
            }

            Label {
                text: "This stores your vault key on this device protected by biometrics. " +
                      "Anyone who can authenticate with their fingerprint or face on this device " +
                      "will be able to unlock your vault."
                font.family: Theme.uiFont
                font.pixelSize: 13
                color: Theme.warning
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: "Your master password still works independently. " +
                      "You can disable biometric unlock at any time from this screen."
                font.family: Theme.uiFont
                font.pixelSize: 12
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
                    onClicked: biometricConfirmPopup.close()
                }

                GrimButton {
                    text: "Enable"
                    primary: true
                    Layout.fillWidth: true
                    onClicked: {
                        biometricConfirmPopup.close()
                        vault.enableBiometric(biometricPasswordField.text)
                        biometricPasswordField.text = ""
                    }
                }
            }
        }
    }

    Connections {
        target: vault
        function onErrorOccurred(message) { errorLabel.text = message }
    }

    Label {
        id: errorLabel
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.spacingMd
        color: Theme.error
        wrapMode: Text.WordWrap
        font.family: Theme.uiFont
        font.pixelSize: 13
        visible: text.length > 0
    }
}
