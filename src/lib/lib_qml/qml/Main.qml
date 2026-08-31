pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Sourcetrail

ApplicationWindow {
    id: window

    width: 1280
    height: 800
    visible: true
    title: AppShell.title.length > 0 ? AppShell.title : qsTr("Sourcetrail")
    color: Theme.background

    Connections {
        target: AppShell
        function onWindowActivationRequested() {
            window.raise()
            window.requestActivate()
        }
    }

    FileDialog {
        id: openProjectDialog
        title: qsTr("Open Sourcetrail project")
        nameFilters: [qsTr("Sourcetrail projects (*.srctrlprj)")]
        onAccepted: AppShell.loadProject(selectedFile)
    }

    header: Rectangle {
        height: AppShell.projectLoaded ? 40 : 0
        visible: AppShell.projectLoaded
        color: Theme.chrome

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.line
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.padding
            anchors.rightMargin: Theme.padding
            spacing: Theme.spacing

            Button {
                text: qsTr("Open…")
                enabled: !AppShell.indexing
                onClicked: openProjectDialog.open()
            }

            Button {
                text: qsTr("Refresh")
                enabled: !AppShell.indexing
                onClicked: AppShell.refresh(false)
            }

            Button {
                text: qsTr("Reindex all")
                enabled: !AppShell.indexing
                onClicked: AppShell.refresh(true)
            }

            ToolSeparator {}

            Button {
                text: qsTr("Overview")
                onClicked: GraphViewModel.showOverview()
            }

            Label {
                text: qsTr("Group by")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }

            ComboBox {
                implicitWidth: 140
                model: [qsTr("Nothing"), qsTr("File"), qsTr("Namespace")]
                currentIndex: GraphViewModel.grouping
                onActivated: GraphViewModel.grouping = currentIndex
            }

            Item { Layout.fillWidth: true }
        }
    }

    // The start screen and the workspace are mutually exclusive, so only one is ever instantiated.
    Loader {
        anchors.fill: parent
        sourceComponent: AppShell.projectLoaded ? workspace : startScreen
    }

    Component {
        id: startScreen

        ColumnLayout {
            spacing: Theme.spacing * 2

            Item { Layout.fillHeight: true }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Sourcetrail")
                color: Theme.text
                font.pixelSize: 44
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Open project…")
                onClicked: openProjectDialog.open()
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                visible: AppShell.recentProjects.length > 0
                text: qsTr("Recent")
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall
            }

            ListView {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 640
                Layout.preferredHeight: Math.min(contentHeight, 300)
                clip: true
                model: AppShell.recentProjects
                // ListView recycles delegates; without this a long recent list rebuilds them on
                // every flick.
                reuseItems: true

                delegate: ItemDelegate {
                    id: projectDelegate
                    required property string modelData
                    width: ListView.view.width
                    text: modelData
                    onClicked: AppShell.loadProject("file://" + projectDelegate.modelData)

                    // `parent` is not reliably the delegate inside contentItem -- reach the text
                    // through the delegate's own id instead.
                    contentItem: Label {
                        text: projectDelegate.text
                        color: Theme.textDim
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontSize
                        elide: Text.ElideLeft
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    Component {
        id: workspace

        // The code panel joins the graph here once it has a view-model of its own.
        Rectangle {
            color: Theme.panel

            GraphPanel {
                anchors.fill: parent
            }
        }
    }

    footer: Rectangle {
        id: statusBar
        height: 26
        color: Theme.chrome

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: Theme.line
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.padding
            anchors.rightMargin: Theme.padding
            spacing: Theme.spacing

            Label {
                Layout.fillWidth: true
                text: AppShell.indexing ? AppShell.progressMessage : AppShell.status
                color: AppShell.statusIsError ? Theme.warn : Theme.textFaint
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }

            ProgressBar {
                visible: AppShell.indexing
                indeterminate: AppShell.progressPercent < 0
                from: 0
                to: 100
                value: Math.max(0, AppShell.progressPercent)
                implicitWidth: 160
            }

            Button {
                visible: AppShell.indexing
                text: qsTr("Cancel")
                onClicked: AppShell.cancelIndexing()
            }
        }
    }
}
