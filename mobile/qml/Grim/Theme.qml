pragma Singleton
import QtQuick

QtObject {
    readonly property string monoFont: "monospace"
    readonly property string bg: "#0a0a0c"
    readonly property string panel: "#0e0e12"
    readonly property string panelAlt: "#111114"
    readonly property string border: "#331111"
    readonly property string borderMuted: "#333338"
    readonly property string text: "#d4d4dc"
    readonly property string textMuted: "#998877"
    readonly property string textDim: "#665566"
    readonly property string prompt: "#33cc66"
    readonly property string error: "#ff4444"
    readonly property string warning: "#996633"
    readonly property int radius: 3
    readonly property int spacing: 12

    function accent() {
        return typeof vault !== "undefined" ? vault.accentColor : "#cc2200"
    }

    function accentHover() {
        const hex = accent()
        if (hex.length !== 7)
            return "#ee3311"
        const r = Math.min(255, parseInt(hex.substr(1, 2), 16) + 34)
        const g = Math.min(255, parseInt(hex.substr(3, 2), 16) + 17)
        const b = Math.min(255, parseInt(hex.substr(5, 2), 16) + 17)
        return "#" + r.toString(16).padStart(2, "0")
             + g.toString(16).padStart(2, "0")
             + b.toString(16).padStart(2, "0")
    }
}
