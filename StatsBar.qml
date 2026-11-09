pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    required property GitRepository repository

    readonly property int peakWeek: Math.max(1, ...repository.weeklyActivity)
    readonly property int recentCommits: repository.weeklyActivity.reduce((sum, n) => sum + n, 0)

    padding: Theme.spacing * 1.5

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacing * 1.5

        StatCard {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Branches")
            value: root.repository.localBranchCount + root.repository.remoteBranchCount
            caption: qsTr("%1 local · %2 remote")
                .arg(root.repository.localBranchCount).arg(root.repository.remoteBranchCount)
        }
        StatCard {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Commits")
            value: root.repository.truncated
                   ? qsTr("%1+").arg(root.repository.commits.count.toLocaleString(Qt.locale(), "f", 0))
                   : root.repository.commits.count.toLocaleString(Qt.locale(), "f", 0)
            caption: root.repository.branchFilter ? qsTr("on selected branch") : qsTr("across all refs")
        }
        StatCard {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Contributors")
            value: root.repository.contributorCount
            caption: root.repository.tagCount === 1 ? qsTr("1 tag") : qsTr("%1 tags").arg(root.repository.tagCount)
        }
        StatCard {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            title: qsTr("Last commit")
            value: Theme.relativeTime(root.repository.lastCommitDate)
            caption: root.repository.lastCommitDate && !isNaN(root.repository.lastCommitDate)
                     ? Theme.shortDate(root.repository.lastCommitDate) : ""
        }

        Frame {
            Layout.fillWidth: true
            Layout.preferredWidth: 2
            Layout.fillHeight: true
            padding: Theme.spacing * 1.5

            Accessible.role: Accessible.Chart
            Accessible.name: qsTr("Commit activity: %1 commits in the last 26 weeks, busiest week %2")
                .arg(root.recentCommits).arg(root.peakWeek)

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Activity · last 26 weeks")
                        color: palette.placeholderText
                        font.pointSize: Theme.fontCaption
                    }
                    Label {
                        text: root.recentCommits === 1 ? qsTr("1 commit") : qsTr("%1 commits").arg(root.recentCommits)
                        color: palette.placeholderText
                        font.pointSize: Theme.fontCaption
                    }
                }

                Item {
                    id: chart
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 36

                    readonly property real barWidth: width / root.repository.weeklyActivity.length

                    Repeater {
                        model: root.repository.weeklyActivity

                        Rectangle {
                            id: bar
                            required property int index
                            required property int modelData

                            x: index * chart.barWidth + 1
                            width: Math.max(1, chart.barWidth - 2)
                            height: modelData > 0 ? Math.max(2, chart.height * modelData / root.peakWeek) : 1
                            anchors.bottom: parent.bottom
                            radius: 1
                            color: modelData > 0 ? palette.highlight : palette.mid

                            HoverHandler { id: barHover }
                            ToolTip.visible: barHover.hovered
                            ToolTip.text: index === root.repository.weeklyActivity.length - 1
                                          ? qsTr("This week: %1").arg(modelData)
                                          : qsTr("%1 weeks ago: %2")
                                                .arg(root.repository.weeklyActivity.length - 1 - index).arg(modelData)
                        }
                    }
                }
            }
        }
    }
}
