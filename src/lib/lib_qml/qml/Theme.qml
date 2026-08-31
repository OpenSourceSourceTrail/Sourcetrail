pragma Singleton
import QtQuick
import Sourcetrail

/**
 * The one place colours, type and metrics are defined.
 *
 * Every value here is lifted from the UX design (new_gui/design/mockup.dc.html, the same document
 * as the linked Claude Design file). Anything that needs a colour, a size or a gap binds to this
 * rather than hard-coding one, so the whole shell can be re-themed from a single file -- and so the
 * design can be diffed against the code by reading one screen of QML.
 *
 * The design is flat by construction: hairline borders, no elevation, no rounded corners. Two
 * separator weights carry the whole hierarchy -- `line` inside a panel, `lineStrong` around
 * controls -- so resist adding shadows or radii to individual components.
 */
QtObject {
    id: theme

    // ---- Surfaces, darkest to lightest -------------------------------------------------------
    readonly property color backdrop: "#0d0f11"        // outside the window; drop shadows land on it
    readonly property color chrome: "#101315"       // title bar, text inputs, the graph canvas
    readonly property color panel: "#131719"        // menu bar, status bar, side panels
    readonly property color panelAlt: "#14181b"     // context panel, code panel, popovers
    readonly property color background: "#16191c"   // the window body
    readonly property color card: "#171c21"         // graph node boxes, table header rows
    readonly property color raised: "#1a2027"       // hover
    readonly property color selected: "#1a2530"     // current row / active tab / accent-tinted fill
    readonly property color chip: "#1d2d3d"         // search chips

    // ---- Lines -------------------------------------------------------------------------------
    readonly property color line: "#23292f"         // inside a panel
    readonly property color lineStrong: "#2c333a"   // around a control
    readonly property color accentLine: "#2c455d"   // around an active control
    readonly property color cornerMark: "#3d4750"   // the drafting `+` marks at panel corners
    readonly property color edge: "#46525c"         // graph edges, legend swatches

    // ---- Text, faintest to brightest ---------------------------------------------------------
    readonly property color gutter: "#4d5761"       // code line numbers
    readonly property color textFainter: "#5f6d78"  // section labels
    readonly property color textFaint: "#6f7c87"    // captions, paths, secondary counts
    readonly property color iconMuted: "#7d8a95"    // inactive rail icons
    readonly property color textMuted: "#93a0ac"    // labels, descriptions
    readonly property color textDim: "#b9c3cb"      // list rows
    readonly property color text: "#e6e9ec"         // primary

    // ---- Accent and state --------------------------------------------------------------------
    readonly property color accent: "#749dc4"
    readonly property color accentHi: "#94bce3"     // hover on accent
    readonly property color accentText: "#101315"     // text on an accent fill
    readonly property color selection: "#2c455d"    // text selection, active code location
    readonly property color ok: "#7fae94"
    readonly property color caution: "#c39a52"      // recoverable errors, "index out of date"
    readonly property color warn: "#c9805f"         // fatal errors
    readonly property color warnBg: "#221a15"
    readonly property color warnLine: "#3d3128"
    readonly property color scrim: Qt.rgba(9 / 255, 11 / 255, 13 / 255, 0.7)

    // ---- Type --------------------------------------------------------------------------------
    // Vendored in bin/app/data/fonts and registered by qml::loadApplicationFonts() before the
    // engine loads. The fallbacks matter: if a family is missing the app must still be legible.
    readonly property string displayFamily: "Barlow Condensed"  // headings, section labels
    readonly property string bodyFamily: "Barlow"
    readonly property string monoFamily: "JetBrains Mono"
    readonly property string displayFallback: "Barlow Condensed, Barlow, sans-serif"
    readonly property string monoFallback: "JetBrains Mono, Source Code Pro, monospace"

    // The design is drawn at a 13px body. The Preferences "Interface font size" control writes
    // AppShell.uiFontSize, and the whole ramp scales from it, so 13 reproduces the mockup exactly
    // and any other value scales proportionally rather than breaking the layout.
    readonly property real designBodySize: 13
    readonly property real scale: AppShell.uiFontSize / theme.designBodySize

    function sized(px) { return Math.round(px * theme.scale) }

    readonly property int fontMicro: theme.sized(11)      // badges, keycaps
    readonly property int fontTiny: theme.sized(11.5)     // table headers, node kind labels
    readonly property int fontSmall: theme.sized(12)      // status detail
    readonly property int fontCode: theme.sized(12.5)     // code, paths, counts
    readonly property int fontSize: theme.sized(13)       // body -- the default
    readonly property int fontBody: theme.sized(13.5)     // list rows, descriptions
    readonly property int fontMedium: theme.sized(14)     // buttons, settings rows
    readonly property int fontLarge: theme.sized(14.5)    // step titles
    readonly property int fontLead: theme.sized(15)       // primary actions
    readonly property int fontIntro: theme.sized(16)      // card titles, empty-state prose
    readonly property int fontStat: theme.sized(20)       // summary figures
    readonly property int fontHeadingS: theme.sized(22)
    readonly property int fontHeadingM: theme.sized(26)
    readonly property int fontHeadingL: theme.sized(30)
    readonly property int fontDisplay: theme.sized(38)    // "NOTHING INDEXED YET"
    readonly property int fontDisplayXl: theme.sized(44)  // "SOURCETRAIL"

    // Tracking, in the em units the design specifies. Qt wants pixels, so multiply by the size.
    readonly property real trackLabel: 0.14     // uppercase section labels
    readonly property real trackEyebrow: 0.16   // the app eyebrow
    readonly property real trackBadge: 0.12     // node kind badges
    readonly property real trackHeading: 0.03

    function tracking(em, pixelSize) { return em * pixelSize }

    // ---- Metrics -----------------------------------------------------------------------------
    readonly property int spacing: 8
    readonly property int spacingTight: 4
    readonly property int spacingWide: 14
    readonly property int padding: 12
    readonly property int paddingWide: 18

    readonly property int titleBarHeight: 34
    readonly property int menuHeight: 30
    readonly property int toolbarHeight: 52
    readonly property int statusHeight: 30
    readonly property int controlHeight: 32
    readonly property int iconButtonSize: 30
    readonly property int railWidth: 52
    readonly property int railButtonSize: 36
    readonly property int panelWidth: 268
    readonly property int codeWidth: 520
    readonly property int panelHeaderHeight: 40
    readonly property int errorsDockHeight: 300
    readonly property int gutterWidth: 44

    // The design uses no radii anywhere. Named so the intent is explicit rather than accidental.
    readonly property int radius: 0
}
