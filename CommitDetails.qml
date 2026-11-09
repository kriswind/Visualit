pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    required property GitRepository repository

    readonly property var details: repository.details
    readonly property bool hasCommit: details.sha !== undefined

    signal parentActivated(string sha)

    padding: Theme.spacing * 2

    BusyIndicator {
        anchors.centerIn: parent
        running: root.repository.detailsLoading && !root.hasCommit
        visible: running
    }

    Label {
        anchors.centerIn: parent
        width: parent.width
        visible: !root.hasCommit && !root.repository.detailsLoading
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: root.details.error ?? qsTr("Select a commit to see its details.")
        color: root.details.error ? Theme.warning : palette.placeholderText
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        visible: root.hasCommit
        contentWidth: availableWidth

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Theme.spacing * 1.5

            Label {
                Layout.fillWidth: true
                text: root.details.subject ?? ""
                wrapMode: Text.Wrap
                font.pointSize: Theme.fontHeading
                font.weight: Font.Medium
            }

            Flow {
                Layout.fillWidth: true
                spacing: 4
                visible: (root.details.refs ?? []).length > 0
                Repeater {
                    model: root.details.refs ?? []
                    RefChip {
                        required property var modelData
                        name: modelData.name
                        kind: modelData.kind
                        current: modelData.current
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Open in browser")
                enabled: !!root.details.url
                highlighted: true
                onClicked: root.repository.openCommitInBrowser(root.details.sha)
                ToolTip.visible: hovered
                ToolTip.text: root.details.url || qsTr("No web remote configured")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.spacing * 1.5
                rowSpacing: 4

                component Key: Label {
                    color: palette.placeholderText
                    Layout.alignment: Qt.AlignTop
                }
                component Value: Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WrapAnywhere
                }

                Key { text: qsTr("Commit") }
                TextEdit {
                    Layout.fillWidth: true
                    text: root.details.sha ?? ""
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.WrapAnywhere
                    font.family: Theme.monoFamily
                    color: palette.text
                    selectionColor: palette.highlight
                    selectedTextColor: palette.highlightedText
                    Accessible.name: qsTr("Commit hash")
                }

                Key { text: qsTr("Author") }
                Value { text: qsTr("%1 <%2>").arg(root.details.author ?? "").arg(root.details.authorEmail ?? "") }

                Key { text: qsTr("Authored") }
                Value {
                    text: root.hasCommit
                          ? qsTr("%1 (%2)").arg(Theme.shortDate(root.details.authorDate))
                                           .arg(Theme.relativeTime(root.details.authorDate))
                          : ""
                }

                Key {
                    visible: root.hasCommit && root.details.committer !== root.details.author
                    text: qsTr("Committer")
                }
                Value {
                    visible: root.hasCommit && root.details.committer !== root.details.author
                    text: root.details.committer ?? ""
                }

                Key { text: qsTr("Parents") }
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Label {
                        visible: (root.details.parents ?? []).length === 0
                        text: qsTr("none (root commit)")
                        color: palette.placeholderText
                    }
                    Repeater {
                        model: root.details.parents ?? []
                        Button {
                            required property string modelData
                            flat: true
                            padding: 2
                            text: modelData.substring(0, 8)
                            font.family: Theme.monoFamily
                            onClicked: root.parentActivated(modelData)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Jump to parent %1").arg(modelData)
                        }
                    }
                }
            }

            TextArea {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.details.body ?? ""
                readOnly: true
                wrapMode: TextEdit.Wrap
                font.family: Theme.monoFamily
                Accessible.name: qsTr("Commit message")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing
                Label {
                    Layout.fillWidth: true
                    readonly property int fileCount: (root.details.files ?? []).length
                    text: fileCount === 1 ? qsTr("1 file changed") : qsTr("%1 files changed").arg(fileCount)
                    font.weight: Font.Medium
                }
                Label {
                    text: qsTr("+%1").arg(root.details.added ?? 0)
                    color: Theme.added
                }
                Label {
                    text: qsTr("−%1").arg(root.details.removed ?? 0)
                    color: Theme.removed
                }
            }

            Repeater {
                model: root.details.files ?? []

                RowLayout {
                    id: fileRow
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.spacing

                    Label {
                        Layout.fillWidth: true
                        text: fileRow.modelData.path
                        elide: Text.ElideLeft
                        font.pointSize: Theme.fontCaption
                        ToolTip.visible: fileHover.hovered && truncated
                        ToolTip.text: fileRow.modelData.path
                        HoverHandler { id: fileHover }
                    }
                    Label {
                        visible: fileRow.modelData.binary
                        text: qsTr("binary")
                        font.pointSize: Theme.fontCaption
                        color: palette.placeholderText
                    }
                    Label {
                        visible: !fileRow.modelData.binary
                        text: "+" + fileRow.modelData.added
                        font.pointSize: Theme.fontCaption
                        color: Theme.added
                    }
                    Label {
                        visible: !fileRow.modelData.binary
                        text: "−" + fileRow.modelData.removed
                        font.pointSize: Theme.fontCaption
                        color: Theme.removed
                    }
                }
            }
        }
    }
}
