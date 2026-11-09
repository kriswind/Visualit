#include "branchmodel.h"

using namespace Qt::StringLiterals;

int BranchModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_branches.size());
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};

    const Branch &b = m_branches.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return b.name;
    case RefNameRole:
        return b.refName;
    case ShaRole:
        return b.sha;
    case SubjectRole:
        return b.subject;
    case DateRole:
        return b.date;
    case IsRemoteRole:
        return b.isRemote;
    case IsCurrentRole:
        return b.isCurrent;
    case SectionRole:
        // Section keys are untranslated identifiers; the view maps them to labels.
        return b.isRemote ? u"remote"_s : u"local"_s;
    }
    return {};
}

QHash<int, QByteArray> BranchModel::roleNames() const
{
    return {
        { NameRole, "name" },         { RefNameRole, "refName" }, { ShaRole, "sha" },
        { SubjectRole, "subject" },   { DateRole, "date" },       { IsRemoteRole, "isRemote" },
        { IsCurrentRole, "isCurrent" }, { SectionRole, "section" },
    };
}

void BranchModel::setBranches(QList<Branch> branches)
{
    // Local branches first, so ListView sections stay contiguous; recency within each group.
    std::stable_sort(branches.begin(), branches.end(), [](const Branch &a, const Branch &b) {
        if (a.isRemote != b.isRemote)
            return !a.isRemote;
        return a.date > b.date;
    });

    beginResetModel();
    m_branches = std::move(branches);
    endResetModel();
    emit countChanged();
}

int BranchModel::indexOfRef(const QString &refName) const
{
    for (int i = 0; i < m_branches.size(); ++i) {
        if (m_branches.at(i).refName == refName)
            return i;
    }
    return -1;
}

QString BranchModel::refNameAt(int row) const
{
    return row >= 0 && row < m_branches.size() ? m_branches.at(row).refName : QString();
}
