pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Sourcetrail

/**
 * The overview page: what the project contains and where the user has been.
 *
 * The per-kind symbol counts the design shows need a storage query that has no view-model yet, so
 * for now this offers the overview action itself plus the live history. Both are real.
 */
ColumnLayout {
    id: root

    spacing: Theme.spacing

    StButton {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.spacingWide
        Layout.rightMargin: Theme.spacingWide
        Layout.topMargin: Theme.spacingWide
        primary: true
        text: qsTr("Show overview graph")
        onClicked: GraphViewModel.showOverview()
    }

    StSectionLabel {
        Layout.leftMargin: Theme.spacingWide
        Layout.topMargin: Theme.spacing
        text: qsTr("Recently visited")
    }

    HistoryPage {
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
