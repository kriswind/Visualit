#pragma once

#include <QDateTime>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>

#include "branchmodel.h"
#include "commitmodel.h"

class GitRepository : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString name READ name NOTIFY pathChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY pathChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

    Q_PROPERTY(QString currentBranch READ currentBranch NOTIFY refsChanged)
    Q_PROPERTY(QString remoteUrl READ remoteUrl NOTIFY remoteChanged)
    Q_PROPERTY(QString webUrl READ webUrl NOTIFY remoteChanged)
    Q_PROPERTY(QString webUrlOverride READ webUrlOverride WRITE setWebUrlOverride NOTIFY remoteChanged)

    // Full ref name to restrict the log to; empty shows every branch and tag.
    Q_PROPERTY(QString branchFilter READ branchFilter WRITE setBranchFilter NOTIFY branchFilterChanged)
    Q_PROPERTY(int maxCommits READ maxCommits WRITE setMaxCommits NOTIFY maxCommitsChanged)

    Q_PROPERTY(CommitModel *commits READ commits CONSTANT)
    Q_PROPERTY(BranchModel *branches READ branches CONSTANT)

    Q_PROPERTY(int localBranchCount READ localBranchCount NOTIFY refsChanged)
    Q_PROPERTY(int remoteBranchCount READ remoteBranchCount NOTIFY refsChanged)
    Q_PROPERTY(int tagCount READ tagCount NOTIFY refsChanged)
    Q_PROPERTY(bool truncated READ isTruncated NOTIFY statsChanged)
    Q_PROPERTY(int contributorCount READ contributorCount NOTIFY statsChanged)
    Q_PROPERTY(QDateTime lastCommitDate READ lastCommitDate NOTIFY statsChanged)
    Q_PROPERTY(QList<int> weeklyActivity READ weeklyActivity NOTIFY statsChanged)

    // Details of the commit last requested via loadDetails(); empty until loaded.
    Q_PROPERTY(QVariantMap details READ details NOTIFY detailsChanged)
    Q_PROPERTY(bool detailsLoading READ detailsLoading NOTIFY detailsChanged)

public:
    static constexpr int ActivityWeeks = 26;

    explicit GitRepository(QObject *parent = nullptr);

    QString path() const { return m_path; }
    QString name() const;
    bool isValid() const { return !m_path.isEmpty(); }
    QString error() const { return m_error; }
    bool isLoading() const { return m_pending > 0; }

    QString currentBranch() const { return m_currentBranch; }
    QString remoteUrl() const { return m_remoteUrl; }
    QString webUrl() const;
    QString webUrlOverride() const { return m_webUrlOverride; }
    void setWebUrlOverride(const QString &url);

    QString branchFilter() const { return m_branchFilter; }
    void setBranchFilter(const QString &refName);
    int maxCommits() const { return m_maxCommits; }
    void setMaxCommits(int count);

    CommitModel *commits() { return &m_commits; }
    BranchModel *branches() { return &m_branches; }

    int localBranchCount() const { return m_localBranchCount; }
    int remoteBranchCount() const { return m_remoteBranchCount; }
    int tagCount() const { return m_tagCount; }
    bool isTruncated() const { return m_truncated; }
    int contributorCount() const { return m_contributorCount; }
    QDateTime lastCommitDate() const { return m_lastCommitDate; }
    QList<int> weeklyActivity() const { return m_weeklyActivity; }

    QVariantMap details() const { return m_details; }
    bool detailsLoading() const { return m_detailsLoading; }

    // Accepts a local path or a file:// URL anywhere inside a working tree.
    Q_INVOKABLE void open(const QString &pathOrUrl);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadDetails(const QString &sha);
    Q_INVOKABLE QString commitUrl(const QString &sha) const;
    // Opens the commit's page on the remote's web host. Emits message() with the outcome.
    Q_INVOKABLE bool openCommitInBrowser(const QString &sha);

    static QString webUrlFromRemote(const QString &remote);

signals:
    void pathChanged();
    void errorChanged();
    void loadingChanged();
    void refsChanged();
    void remoteChanged();
    void branchFilterChanged();
    void maxCommitsChanged();
    void statsChanged();
    void detailsChanged();
    void message(const QString &text);

private:
    using GitCallback = std::function<void(int exitCode, const QByteArray &out, const QByteArray &err)>;
    void runGit(const QString &workingDir, const QStringList &args, GitCallback done);
    void beginTask();
    void endTask();
    void setError(const QString &error);

    void loadRefs();
    void loadRemote();
    void loadLog();

    void parseRefs(const QByteArray &out);
    void parseLog(const QByteArray &out);
    void parseDetails(const QByteArray &out);

    QString m_path;
    QString m_error;
    int m_pending = 0;
    // Bumped whenever results of in-flight git calls become stale.
    quint64 m_generation = 0;
    quint64 m_logGeneration = 0;
    quint64 m_detailsGeneration = 0;

    QString m_currentBranch;
    QString m_remoteUrl;
    QString m_webUrlOverride;
    QString m_branchFilter;
    int m_maxCommits = 5000;

    CommitModel m_commits;
    BranchModel m_branches;

    int m_localBranchCount = 0;
    int m_remoteBranchCount = 0;
    int m_tagCount = 0;
    bool m_truncated = false;
    int m_contributorCount = 0;
    QDateTime m_lastCommitDate;
    QList<int> m_weeklyActivity;

    QVariantMap m_details;
    bool m_detailsLoading = false;
};
