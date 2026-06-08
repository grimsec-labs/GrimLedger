import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 720
    title: "GrimLedger"
    color: "#0a0a0c"

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: loginScreen
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
                background: Rectangle { color: "#111114" }
                TabButton {
                    text: "Notes"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? vault.accentColor : "#998877"
                        horizontalAlignment: Text.AlignHCenter
                        font.family: "monospace"
                    }
                }
                TabButton {
                    text: "Keys"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? vault.accentColor : "#998877"
                        horizontalAlignment: Text.AlignHCenter
                        font.family: "monospace"
                    }
                }
                TabButton {
                    text: "Settings"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? vault.accentColor : "#998877"
                        horizontalAlignment: Text.AlignHCenter
                        font.family: "monospace"
                    }
                }
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
