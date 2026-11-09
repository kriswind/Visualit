#include "commitmodel.h"

using namespace Qt::StringLiterals;

int CommitModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_commits.size());
}

QVariant CommitModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};

    const Commit &c = m_commits.at(index.row());
    switch (role) {
    case ShaRole:
        return c.sha;
    case ShortShaRole:
        return c.sha.left(8);
    case Qt::DisplayRole:
    case SubjectRole:
        return c.subject;
    case AuthorRole:
        return c.author;
    case EmailRole:
        return c.email;
    case DateRole:
        return c.date;
    case RefsRole:
        return c.refs;
    case ParentsRole:
        return c.parents;
    case LaneRole:
        return c.lane;
    case SegmentsRole:
        return QVariant::fromValue(c.segments);
    case IsMergeRole:
        return c.parents.size() > 1;
    case IsHeadRole:
        return c.isHead;
    }
    return {};
}

QHash<int, QByteArray> CommitModel::roleNames() const
{
    return {
        { ShaRole, "sha" },         { ShortShaRole, "shortSha" }, { SubjectRole, "subject" },
        { AuthorRole, "author" },   { EmailRole, "email" },       { DateRole, "date" },
        { RefsRole, "refs" },       { ParentsRole, "parents" },   { LaneRole, "lane" },
        { SegmentsRole, "segments" }, { IsMergeRole, "isMerge" }, { IsHeadRole, "isHead" },
    };
}

void CommitModel::setCommits(QList<Commit> commits)
{
    beginResetModel();
    m_maxLanes = layoutGraph(commits);
    m_commits = std::move(commits);
    m_rowBySha.clear();
    m_rowBySha.reserve(m_commits.size());
    for (int i = 0; i < m_commits.size(); ++i)
        m_rowBySha.insert(m_commits.at(i).sha, i);
    endResetModel();
    emit countChanged();
}

int CommitModel::indexOfSha(const QString &shaPrefix) const
{
    if (shaPrefix.isEmpty())
        return -1;
    if (auto it = m_rowBySha.constFind(shaPrefix); it != m_rowBySha.cend())
        return *it;
    for (int i = 0; i < m_commits.size(); ++i) {
        if (m_commits.at(i).sha.startsWith(shaPrefix))
            return i;
    }
    return -1;
}

QString CommitModel::shaAt(int row) const
{
    return row >= 0 && row < m_commits.size() ? m_commits.at(row).sha : QString();
}

int CommitModel::find(const QString &text, int from, bool forward) const
{
    const QString needle = text.trimmed();
    const int n = int(m_commits.size());
    if (needle.isEmpty() || n == 0)
        return -1;

    const auto matches = [&needle](const Commit &c) {
        if (c.subject.contains(needle, Qt::CaseInsensitive)
            || c.author.contains(needle, Qt::CaseInsensitive)
            || c.email.contains(needle, Qt::CaseInsensitive)
            || c.sha.startsWith(needle, Qt::CaseInsensitive))
            return true;
        for (const QVariant &ref : c.refs) {
            if (ref.toMap().value(u"name"_s).toString().contains(needle, Qt::CaseInsensitive))
                return true;
        }
        return false;
    };

    const int step = forward ? 1 : -1;
    int row = ((from % n) + n) % n;
    for (int i = 0; i < n; ++i, row = (row + step + n) % n) {
        if (matches(m_commits.at(row)))
            return row;
    }
    return -1;
}

// Assigns each commit a lane and records the line segments to draw in its row.
// Commits must be in topological order (children before parents). `lanes` holds,
// per column, the sha of the commit that column is waiting for.
int CommitModel::layoutGraph(QList<Commit> &commits)
{
    QStringList lanes;
    int maxLanes = 0;

    for (Commit &c : commits) {
        QList<int> segs;

        int col = int(lanes.indexOf(c.sha));
        if (col < 0) {
            col = int(lanes.indexOf(QString()));
            if (col < 0) {
                col = int(lanes.size());
                lanes.append(QString());
            }
        }

        // Top half: lines entering the row. Lanes waiting for this commit converge on its node.
        QList<bool> passThrough(lanes.size(), false);
        for (int i = 0; i < lanes.size(); ++i) {
            if (lanes.at(i).isEmpty())
                continue;
            if (lanes.at(i) == c.sha) {
                segs << 0 << i << col << i;
                lanes[i].clear();
            } else {
                segs << 0 << i << i << i;
                passThrough[i] = true;
            }
        }

        // Bottom half: lines leaving the row toward parents, plus unaffected lanes.
        if (!c.parents.isEmpty()) {
            lanes[col] = c.parents.first();
            segs << 1 << col << col << col;
            for (qsizetype p = 1; p < c.parents.size(); ++p) {
                const QString &parent = c.parents.at(p);
                int target = int(lanes.indexOf(parent));
                if (target < 0) {
                    target = int(lanes.indexOf(QString()));
                    if (target < 0) {
                        target = int(lanes.size());
                        lanes.append(parent);
                    } else {
                        lanes[target] = parent;
                    }
                }
                segs << 1 << col << target << target;
            }
        }
        for (int i = 0; i < passThrough.size(); ++i) {
            if (passThrough.at(i))
                segs << 1 << i << i << i;
        }

        c.lane = col;
        c.segments = std::move(segs);
        maxLanes = std::max(maxLanes, int(lanes.size()));

        while (!lanes.isEmpty() && lanes.constLast().isEmpty())
            lanes.removeLast();
    }
    return maxLanes;
}
