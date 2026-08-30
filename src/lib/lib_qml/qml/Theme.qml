pragma Singleton
import QtQuick

/**
 * The one place colours and metrics are defined.
 *
 * Values come from the dark scheme the redesign mockup specifies: flat, dense, hairline-separated,
 * no elevation. Anything that needs a colour binds to this rather than hard-coding one, so the
 * whole shell can be re-themed from a single file.
 */
QtObject {
    readonly property color background: "#16191c"
    readonly property color chrome: "#101315"
    readonly property color panel: "#131719"
    readonly property color raised: "#1a2027"
    readonly property color selected: "#1a2530"

    readonly property color line: "#23292f"
    readonly property color lineStrong: "#2c333a"

    readonly property color text: "#e6e9ec"
    readonly property color textDim: "#b9c3cb"
    readonly property color textMuted: "#93a0ac"
    readonly property color textFaint: "#6f7c87"

    readonly property color accent: "#749dc4"
    readonly property color accentHi: "#94bce3"
    readonly property color warn: "#c9805f"
    readonly property color ok: "#7fae94"

    readonly property int fontSize: 13
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeLarge: 17
    readonly property string monoFamily: "Source Code Pro"

    readonly property int spacing: 8
    readonly property int padding: 12
    readonly property int controlHeight: 32
}
