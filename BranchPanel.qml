pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    required property GitRepository repository

    signal branchActivated()

    padding: 0

    function activate(refName: string) {
        repository.branchFilter = refName
        branchActivated()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            id: allBranches
            Layout.fillWidth: true
            highlighted: root.repository.branchFilter === ""
            text: qsTr("All branches")
            font.weight: Font.Medium
            onClicked: root.activate("")
            KeyNavigation.down: branchList
            Keys.onReturnPressed: root.activate("")
            Keys.onEnterPressed: root.activate("")
        }

        MenuSeparator { Layout.fillWidth: true }

        ListView {
            id: branchList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.repository.branches
            currentIndex: root.repository.branches.indexOfRef(root.repository.branchFilter)
            keyNavigationEnabled: true
            activeFocusOnTab: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            Keys.onReturnPressed: if (currentIndex >= 0) root.activate(root.repository.branches.refNameAt(currentIndex))
            Keys.onEnterPressed: if (currentIndex >= 0) root.activate(root.repository.branches.refNameAt(currentIndex))
            Keys.onUpPressed: event => {
                if (currentIndex <= 0)
                    allBranches.forceActiveFocus()
                else
                    event.accepted = false
            }

            section.property: "section"
            section.criteria: ViewSection.FullString
            section.delegate: Label {
                required property string section
                width: ListView.view.width
                topPadding: Theme.spacing * 1.5
                bottomPadding: Theme.spacing / 2
                leftPadding: Theme.spacing * 1.5
                text: section === "remote" ? qsTr("REMOTE") : qsTr("LOCAL")
                color: palette.placeholderText
                font.pointSize: Theme.fontCaption
                font.weight: Font.Medium
            }

            delegate: ItemDelegate {
                id: branchItem

                required property int index
                required property string name
                required property string refName
                required property string subject
                required property date date
                required property bool isRemote
                required property bool isCurrent

                width: ListView.view.width
                highlighted: ListView.isCurrentItem
                onClicked: {
                    branchList.currentIndex = index
                    root.activate(refName)
                }

                Accessible.name: isCurrent ? qsTr("%1, checked out").arg(name) : name

                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: subject

                contentItem: ColumnLayout {
                    spacing: 0
                    RowLayout {
                        spacing: Theme.spacing / 2
                        Label {
                            visible: branchItem.isCurrent
                            text: "●"
                            color: palette.highlight
                            Accessible.ignored: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: branchItem.name
                            elide: Text.ElideMiddle
                            font.weight: branchItem.isCurrent ? Font.Medium : Font.Normal
                            color: branchItem.highlighted ? palette.highlightedText : palette.text
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: Theme.relativeTime(branchItem.date)
                        elide: Text.ElideRight
                        font.pointSize: Theme.fontCaption
                        color: branchItem.highlighted ? palette.highlightedText : palette.placeholderText
                    }
                }
            }
        }
    }
}
