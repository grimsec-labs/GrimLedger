import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Grim

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 720
    title: "GrimLedger"
    color: Theme.bg

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: loginHost
    }

    Connections {
        target: vault
        function onUnlockedChanged() {
            if (!vault.unlocked)
                stack.replace(loginHost)
        }
    }

    Component {
        id: loginHost
        Item {
            implicitWidth: stack.width
            implicitHeight: stack.height
            Loader {
                anchors.fill: parent
                source: "LoginScreen.qml"
                onLoaded: {
                    if (item)
                        item.unlocked.connect(function() { stack.replace(mainShell) })
                }
            }
        }
    }

    Component {
        id: mainShell
        ColumnLayout {
            spacing: 0
            implicitWidth: stack.width
            implicitHeight: stack.height

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.panelAlt
                border.color: Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10
                    Image {
                        source: "qrc:/logo.png"
                        fillMode: Image.PreserveAspectFit
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                    }
                    Label {
                        text: "GRIMLEDGER"
                        font.family: Theme.monoFont
                        font.bold: true
                        font.letterSpacing: 2
                        color: Theme.accent()
                        Layout.fillWidth: true
                    }
                }
            }

            TabBar {
                id: tabs
                Layout.fillWidth: true
                background: Rectangle { color: Theme.panelAlt }

                TabButton {
                    text: "Notes"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? Theme.accent() : Theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Theme.monoFont
                        font.bold: parent.checked
                    }
                }
                TabButton {
                    text: "Keys"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? Theme.accent() : Theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Theme.monoFont
                        font.bold: parent.checked
                    }
                }
                TabButton {
                    text: "Settings"
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? Theme.accent() : Theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        font.family: Theme.monoFont
                        font.bold: parent.checked
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabs.currentIndex

                Loader { source: "NotesScreen.qml"; Layout.fillWidth: true; Layout.fillHeight: true }
                Loader { source: "CredentialsScreen.qml"; Layout.fillWidth: true; Layout.fillHeight: true }
                Loader { source: "SettingsScreen.qml"; Layout.fillWidth: true; Layout.fillHeight: true }
            }
        }
    }
}
