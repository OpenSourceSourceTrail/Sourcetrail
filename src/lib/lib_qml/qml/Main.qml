pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Sourcetrail

ApplicationWindow {
    id: window

    width: 1600
    height: 1000
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

        AppFrame {
            onOpenProjectRequested: openProjectDialog.open()
            onRefreshRequested: AppShell.refresh(false)
        }
    }

    // Ctrl+K on Linux and Windows, Command+K on macOS. StandardKey has no palette entry, so the
    // sequence is spelled out; Qt maps "Ctrl" to Command on macOS automatically.
    Shortcut {
        sequences: ["Ctrl+K"]
        onActivated: console.log("command palette arrives with the search view-model")
    }
}
