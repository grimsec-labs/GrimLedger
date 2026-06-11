import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0
import controls 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 720
    title: "GrimLedger"
    color: Theme.background
    font.family: Theme.uiFont

    Binding {
        target: Theme
        property: "accent"
        value: vault.accentColor
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: loginScreen
    }

    Connections {
        target: vault
        function onUnlockedChanged() {
            if (!vault.unlocked)
                stack.replace(loginScreen)
        }
    }

    Component {
        id: loginScreen
        LoginScreen {
            onUnlocked: stack.replace(mainShell)
        }
    }

    Component {
        id: mainShell
        ColumnLayout {
            spacing: 0

            TabBar {
                id: tabs
                Layout.fillWidth: true
                implicitHeight: Theme.touchTarget

                background: Rectangle {
                    color: Theme.surfaceRaised
                    border.bottom.color: Theme.borderInfernal
                    border.bottom.width: 1
                }

                GrimTabButton { text: "Notes" }
                GrimTabButton { text: "Keys" }
                GrimTabButton { text: "Settings" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabs.currentIndex

                NotesScreen {}
                CredentialsScreen {}
                SettingsScreen {}
            }
        }
    }
}
