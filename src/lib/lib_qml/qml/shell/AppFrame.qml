pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Sourcetrail

/**
 * The workspace: menu bar, toolbar, rail, context panel, centre, code panel, status bar.
 *
 * Proportions come straight from the design -- a 52px rail, a 268px context panel, a 520px code
 * panel, and the graph taking whatever is left. Only the graph is live at this point; the code
 * panel and the errors dock arrive with their own view-models.
 */
Item {
    id: root

    signal openProjectRequested()
    signal newProjectRequested()
    signal refreshRequested()
    signal preferencesRequested()

    // Ctrl on Linux and Windows, Command on macOS -- shown as the design writes it.
    readonly property string paletteShortcutLabel: Qt.platform.os === "osx" ? "⌘K" : "Ctrl+K"

    property bool trailOpen: false
    property bool legendOpen: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        MenuBar {
            Layout.fillWidth: true
            onOpenProjectRequested: root.openProjectRequested()
            onNewProjectRequested: root.newProjectRequested()
            onRefreshRequested: root.refreshRequested()
            onPreferencesRequested: root.preferencesRequested()
            onLegendRequested: root.legendOpen = true
        }

        Toolbar {
            id: toolbar
            Layout.fillWidth: true
            trailOpen: root.trailOpen
            paletteShortcutLabel: root.paletteShortcutLabel
            onPaletteRequested: SearchViewModel.paletteOpen = true
            onTrailToggled: root.trailOpen = !root.trailOpen
            onErrorsRequested: rail.currentPage = rail.pageErrors
            onPreferencesRequested: root.preferencesRequested()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            IconRail {
                id: rail
                Layout.fillHeight: true
                onLegendRequested: root.legendOpen = true
            }

            ContextPanel {
                Layout.fillHeight: true
                currentPage: rail.currentPage
                projectName: AppShell.projectSummary
            }

            GraphPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            CodePanel {
                Layout.preferredWidth: Theme.codeWidth
                Layout.fillHeight: true
            }
        }

        StatusBar {
            Layout.fillWidth: true
            paletteShortcutLabel: root.paletteShortcutLabel
            onErrorsRequested: rail.currentPage = rail.pageErrors
        }
    }

    CommandPalette {}

    // A controller asking to be seen -- StatusController and ErrorController both do this -- picks
    // the matching rail entry. View names come from View::getName().
    Connections {
        target: StatusViewModel
        function onViewRaiseRequested(viewName) {
            if (viewName === "Errors")
                rail.currentPage = rail.pageErrors
        }
    }
}
