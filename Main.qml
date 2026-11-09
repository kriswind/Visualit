import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    required property string initialPath
    required property string webUrlOverride
    property bool compactView: false

    width: 1440
    height: 880
    minimumWidth: 900
    minimumHeight: 520
    visible: true
    title: repo.valid ? qsTr("%1 — Visualit").arg(repo.name) : qsTr("Visualit")

    GitRepository {
        id: repo
        webUrlOverride: window.webUrlOverride
        onMessage: text => statusBar.show(text)
        onErrorChanged: if (error) statusBar.show(error)
    }

    Component.onCompleted: {
        repo.open(initialPath)
        commitList.focusList()
    }

    header: AppToolBar {
        id: toolBar
        repository: repo
        compact: window.compactView
        onCompactToggled: window.compactView = !window.compactView
        onOpenRequested: folderDialog.open()
        onSearchRequested: (text, forward) => commitList.search(text, forward)
        onSearchDismissed: commitList.focusList()
    }

    footer: StatusBar {
        id: statusBar
        repository: repo
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StatsBar {
            Layout.fillWidth: true
            repository: repo
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            BranchPanel {
                SplitView.preferredWidth: 280
                SplitView.minimumWidth: 180
                repository: repo
                onBranchActivated: commitList.focusList()
            }

            CommitList {
                id: commitList
                compact: window.compactView
                SplitView.fillWidth: true
                SplitView.minimumWidth: 360
                repository: repo
                onSearchFailed: text => statusBar.show(qsTr("No commit matches “%1”.").arg(text))
            }

            CommitDetails {
                SplitView.preferredWidth: 400
                SplitView.minimumWidth: 260
                repository: repo
                onParentActivated: sha => commitList.selectSha(sha)
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Open git repository")
        currentFolder: repo.valid ? "file://" + repo.path : ""
        onAccepted: repo.open(selectedFolder.toString())
    }

    Settings {
        property alias compactView: window.compactView
    }

    Shortcut {
        sequence: "Ctrl+M"
        onActivated: window.compactView = !window.compactView
    }
    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: toolBar.focusSearch()
    }
    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: folderDialog.open()
    }
    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R"]
        onActivated: repo.refresh()
    }
}
