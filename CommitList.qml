pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    required property GitRepository repository
    property bool compact: false

    readonly property int rowHeight: compact ? Theme.compactRowHeight : Theme.rowHeight

    readonly property real graphWidth: Math.min(Math.max(repository.commits.maxLanes, 1), Theme.maxVisibleLanes)
                                       * Theme.laneWidth + Theme.spacing
    readonly property real authorWidth: Math.min(180, width * 0.18)
    readonly property real dateWidth: 150
    readonly property real shaWidth: 84

    signal searchFailed(string text)

    padding: 0
    focusPolicy: Qt.NoFocus

    function focusList() {
        list.forceActiveFocus()
    }

    function selectRow(row: int) {
        if (row < 0)
            return
        list.currentIndex = row
        list.positionViewAtIndex(row, ListView.Contain)
    }

    function selectSha(sha: string) {
        selectRow(repository.commits.indexOfSha(sha))
        list.forceActiveFocus()
    }

    function search(text: string, forward: bool) {
        if (!text.trim())
            return
        const row = repository.commits.find(text, list.currentIndex + (forward ? 1 : -1), forward)
        if (row < 0)
            searchFailed(text)
        else
            selectRow(row)
    }

    function openCurrent() {
        if (list.currentIndex >= 0)
            repository.openCommitInBrowser(repository.commits.shaAt(list.currentIndex))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: header.implicitHeight + Theme.spacing
            color: palette.alternateBase

            Row {
                id: header
                anchors.verticalCenter: parent.verticalCenter
                x: Theme.spacing / 2

                component HeaderLabel: Label {
                    font.pointSize: Theme.fontCaption
                    font.weight: Font.Medium
                    color: palette.placeholderText
                    elide: Text.ElideRight
                }

                HeaderLabel {
                    visible: root.compact
                    width: list.width - Theme.spacing
                    text: qsTr("Graph — click a node to open it in the browser, hover for details")
                }
                HeaderLabel { visible: !root.compact; width: root.graphWidth; text: qsTr("Graph") }
                HeaderLabel {
                    visible: !root.compact
                    width: list.width - root.graphWidth - root.authorWidth - root.dateWidth - root.shaWidth
                           - Theme.spacing
                    text: root.repository.branchFilter
                          ? qsTr("Message — %1").arg(root.repository.branchFilter.replace(/^refs\/(heads|remotes)\//, ""))
                          : qsTr("Message")
                }
                HeaderLabel { visible: !root.compact; width: root.authorWidth; text: qsTr("Author") }
                HeaderLabel { visible: !root.compact; width: root.dateWidth; text: qsTr("Date") }
                HeaderLabel { visible: !root.compact; width: root.shaWidth; text: qsTr("Commit") }
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.repository.commits
            keyNavigationEnabled: true
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            reuseItems: true
            activeFocusOnTab: true
            ScrollBar.vertical: ScrollBar { }

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Commits")

            Keys.onReturnPressed: root.openCurrent()
            Keys.onEnterPressed: root.openCurrent()
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Home) {
                    root.selectRow(0)
                    event.accepted = true
                } else if (event.key === Qt.Key_End) {
                    root.selectRow(count - 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_PageDown || event.key === Qt.Key_PageUp) {
                    const page = Math.max(1, Math.floor(height / root.rowHeight) - 1)
                    const step = event.key === Qt.Key_PageDown ? page : -page
                    root.selectRow(Math.max(0, Math.min(count - 1, currentIndex + step)))
                    event.accepted = true
                }
            }

            onCountChanged: if (count > 0 && currentIndex < 0) currentIndex = 0

            delegate: root.compact ? compactDelegate : fullDelegate

            Component {
                id: fullDelegate
                CommitDelegate {
                    width: list.width
                    graphWidth: root.graphWidth
                    authorWidth: root.authorWidth
                    dateWidth: root.dateWidth
                    shaWidth: root.shaWidth
                    onActivated: root.openCurrent()
                    onClicked: {
                        list.currentIndex = index
                        list.forceActiveFocus()
                    }
                }
            }

            Component {
                id: compactDelegate
                CompactCommitDelegate {
                    width: list.width
                    onClicked: {
                        list.currentIndex = index
                        list.forceActiveFocus()
                    }
                    onNodeClicked: {
                        list.currentIndex = index
                        list.forceActiveFocus()
                        root.openCurrent()
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0 && !root.repository.loading
                text: root.repository.valid ? qsTr("No commits") : qsTr("Open a git repository to begin (Ctrl+O)")
                color: palette.placeholderText
            }
        }
    }

    // Debounce so holding an arrow key does not spawn a git process per row.
    Timer {
        id: detailsTimer
        interval: 120
        onTriggered: root.repository.loadDetails(root.repository.commits.shaAt(list.currentIndex))
    }
    onCompactChanged: Qt.callLater(() => list.positionViewAtIndex(list.currentIndex, ListView.Center))

    Connections {
        target: list
        function onCurrentIndexChanged() { detailsTimer.restart() }
    }
    Connections {
        target: root.repository.commits
        function onModelReset() { detailsTimer.restart() }
    }
}
