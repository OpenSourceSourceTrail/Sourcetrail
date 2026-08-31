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

    color: Theme.textMuted
    font.family: Theme.displayFamily
    font.pixelSize: Theme.fontTiny
    font.weight: Font.Medium
    font.capitalization: Font.AllUppercase
    font.letterSpacing: Theme.tracking(root.tracking, root.font.pixelSize)
    renderType: Text.NativeRendering
}
