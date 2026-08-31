import QtQuick
import Sourcetrail

/**
 * The condensed, uppercase, wide-tracked label the design uses for every section heading --
 * "RECENT PROJECTS", "INDEXING PROBLEMS", "Nodes", "LANGUAGE".
 */
Text {
    id: root

    /** Tracking in em, as the design specifies it. */
    property real tracking: Theme.trackLabel
    /**
     * The size the tracking is computed from. A separate property rather than font.pixelSize:
     * `font` is one grouped object, so a letterSpacing binding that reads font.pixelSize is a
     * binding loop.
     */
    property int size: Theme.fontTiny

    color: Theme.textMuted
    font.family: Theme.displayFamily
    font.pixelSize: root.size
    font.weight: Font.Medium
    font.capitalization: Font.AllUppercase
    font.letterSpacing: Theme.tracking(root.tracking, root.size)
    renderType: Text.NativeRendering
}
