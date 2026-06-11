pragma Singleton
import QtQuick

QtObject {
    id: theme

    readonly property color background: "#0a0a0c"
    readonly property color surface: "#111114"
    readonly property color surfaceRaised: "#0e0e12"
    readonly property color listBackground: "#0c0c10"

    readonly property color border: "#2a2a30"
    readonly property color borderInfernal: "#331111"

    readonly property color textPrimary: "#d4d4dc"
    readonly property color textBright: "#e8e8ec"
    readonly property color textMuted: "#998877"
    readonly property color textDim: "#665566"

    readonly property color terminalGreen: "#33cc66"
    readonly property color error: "#ff4444"
    readonly property color warning: "#ffcc66"

    readonly property color selectionBg: "#442200"
    readonly property color selectionText: "#ffcc88"

    readonly property color primaryButtonBg: "#1a0808"
    readonly property color primaryButtonPressedBg: "#2a1010"
    readonly property color listItemSelectedBg: "#1a1010"

    readonly property color defaultAccent: "#cc2200"

    // Updated from main.qml via Binding to vault.accentColor (singletons cannot read context properties).
    property color accent: defaultAccent
    readonly property color accentHover: Qt.lighter(accent, 1.25)
    readonly property color accentSoft: Qt.darker(accent, 1.50)

    readonly property int radiusSmall: 3
    readonly property int radiusMedium: 6
    readonly property int radiusLarge: 10

    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32

    readonly property int touchTarget: 48

    readonly property string uiFont: Qt.platform.os === "android" ? "Roboto" : "Segoe UI"
    readonly property string monoFont: "monospace"

    readonly property var accentPresets: [
        { name: "Hellfire Red", hex: "#cc2200" },
        { name: "Ember Orange", hex: "#ff6600" },
        { name: "Terminal Green", hex: "#00cc66" },
        { name: "Abyss Purple", hex: "#8844cc" }
    ]

    readonly property color loginGradientStart: "#08080a"
    readonly property color loginGradientMid: "#0e0a0c"
    readonly property color loginGradientEnd: "#0a0808"
}
