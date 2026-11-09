pragma Singleton
import QtQuick

QtObject {
    readonly property bool dark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark

    // Mid-tone hues that stay distinguishable on both light and dark rows.
    readonly property list<color> laneColors: dark
        ? ["#6ea8ff", "#ffa657", "#56d364", "#ff7eb6", "#bc8cff", "#39c5cf", "#e3b341", "#ff7b72"]
        : ["#1f6feb", "#d1590a", "#1a7f37", "#c2187a", "#8250df", "#0e8a96", "#9a6700", "#cf222e"]

    readonly property color localRef: dark ? "#2f5fb3" : "#ddeaff"
    readonly property color localRefText: dark ? "#e6efff" : "#0b3d91"
    readonly property color remoteRefText: dark ? "#8fd6dc" : "#0b6670"
    readonly property color tagRef: dark ? "#5c4813" : "#fff1c2"
    readonly property color tagRefText: dark ? "#f6dd94" : "#6b4a00"
    readonly property color added: dark ? "#56d364" : "#1a7f37"
    readonly property color removed: dark ? "#ff7b72" : "#cf222e"
    readonly property color warning: dark ? "#e3b341" : "#9a6700"

    readonly property int rowHeight: 30
    readonly property int laneWidth: 14
    readonly property int maxVisibleLanes: 16
    readonly property int compactRowHeight: 16
    readonly property int compactLaneWidth: 12
    readonly property int spacing: 8
    readonly property int radius: 6

    // Modular scale, ratio 1.2 around the platform's default point size.
    readonly property real fontBase: Qt.application.font.pointSize
    readonly property real fontCaption: Math.round(fontBase / 1.2 * 10) / 10
    readonly property real fontHeading: Math.round(fontBase * 1.2 * 10) / 10
    readonly property real fontDisplay: Math.round(fontBase * 1.44 * 10) / 10

    readonly property string monoFamily: "monospace"

    function relativeTime(date: date): string {
        if (!date || isNaN(date.getTime()))
            return qsTr("never")
        const seconds = Math.max(0, (Date.now() - date.getTime()) / 1000)
        if (seconds < 60)
            return qsTr("just now")
        const minutes = Math.floor(seconds / 60)
        if (minutes < 60)
            return minutes === 1 ? qsTr("1 minute ago") : qsTr("%1 minutes ago").arg(minutes)
        const hours = Math.floor(minutes / 60)
        if (hours < 24)
            return hours === 1 ? qsTr("1 hour ago") : qsTr("%1 hours ago").arg(hours)
        const days = Math.floor(hours / 24)
        if (days < 30)
            return days === 1 ? qsTr("yesterday") : qsTr("%1 days ago").arg(days)
        const months = Math.floor(days / 30.44)
        if (months < 12)
            return months <= 1 ? qsTr("1 month ago") : qsTr("%1 months ago").arg(months)
        const years = Math.floor(days / 365.25)
        return years <= 1 ? qsTr("1 year ago") : qsTr("%1 years ago").arg(years)
    }

    function shortDate(date: date): string {
        return date.toLocaleString(Qt.locale(), Locale.ShortFormat)
    }
}
