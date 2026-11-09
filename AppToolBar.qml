import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: root

    required property GitRepository repository
    required property bool compact

    signal openRequested()
    signal compactToggled()
    signal searchRequested(string text, bool forward)
    signal searchDismissed()

    function focusSearch() {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing * 1.5
        anchors.rightMargin: Theme.spacing
        spacing: Theme.spacing

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 120
            spacing: 0

            RowLayout {
                spacing: Theme.spacing

                Label {
                    text: root.repository.valid ? root.repository.name : qsTr("No repository")
                    font.pointSize: Theme.fontHeading
                    font.weight: Font.Medium
                }
                Label {
                    visible: root.repository.valid
                    text: root.repository.currentBranch
                          ? qsTr("on %1").arg(root.repository.currentBranch)
                          : qsTr("detached HEAD")
                    color: palette.placeholderText
                }
                BusyIndicator {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    running: root.repository.loading
                    visible: running
                    Accessible.name: qsTr("Loading")
                }
            }
            Label {
                Layout.fillWidth: true
                text: root.repository.path
                elide: Text.ElideMiddle
                color: palette.placeholderText
                font.pointSize: Theme.fontCaption
            }
        }

        TextField {
            id: searchField
            Layout.preferredWidth: 300
            placeholderText: qsTr("Search commits (Ctrl+F)")
            Accessible.name: qsTr("Search commits")
            ToolTip.visible: activeFocus && text.length > 0
            ToolTip.text: qsTr("Enter: next match · Shift+Enter: previous · Esc: back to list")
            ToolTip.delay: 600

            Keys.onReturnPressed: event => root.searchRequested(text, !(event.modifiers & Qt.ShiftModifier))
            Keys.onEnterPressed: event => root.searchRequested(text, !(event.modifiers & Qt.ShiftModifier))
            Keys.onEscapePressed: root.searchDismissed()
        }

        ToolButton {
            text: qsTr("Compact")
            checkable: true
            checked: root.compact
            onClicked: root.compactToggled()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Graph-only view; click a node to open it (Ctrl+M)")
        }
        ToolButton {
            text: qsTr("Open…")
            onClicked: root.openRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Open another repository (Ctrl+O)")
        }
        ToolButton {
            text: qsTr("Refresh")
            enabled: root.repository.valid
            onClicked: root.repository.refresh()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Reload branches and commits (F5)")
        }
    }
}
