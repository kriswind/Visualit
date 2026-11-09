#include "gitrepository.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace {

constexpr QChar FieldSep(0x1f);
constexpr QChar RecordSep(0x1e);

QDateTime fromUnix(const QString &seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds.toLongLong());
}

// Splits a `%D` decoration produced with --decorate=full into typed refs.
QVariantList parseDecorations(const QString &decorations, bool *isHead)
{
    QVariantList refs;
    *isHead = false;
    if (decorations.isEmpty())
        return refs;

    for (QString item : decorations.split(u", "_s, Qt::SkipEmptyParts)) {
        bool current = false;
        if (item == u"HEAD"_s) {
            *isHead = true;
            continue;
        }
        if (item.startsWith(u"HEAD -> "_s)) {
            *isHead = true;
            current = true;
            item = item.mid(8);
        }

        QString kind;
        if (item.startsWith(u"tag: refs/tags/"_s)) {
            kind = u"tag"_s;
            item = item.mid(15);
        } else if (item.startsWith(u"refs/heads/"_s)) {
            kind = u"local"_s;
            item = item.mid(11);
        } else if (item.startsWith(u"refs/remotes/"_s)) {
            item = item.mid(13);
            if (item.endsWith(u"/HEAD"_s))
                continue;
            kind = u"remote"_s;
        } else {
            continue;
        }
        refs.append(QVariantMap{ { u"name"_s, item }, { u"kind"_s, kind }, { u"current"_s, current } });
    }
    return refs;
}

} // namespace

GitRepository::GitRepository(QObject *parent)
    : QObject(parent)
    , m_weeklyActivity(ActivityWeeks, 0)
{
}

QString GitRepository::name() const
{
    return m_path.isEmpty() ? QString() : QFileInfo(m_path).fileName();
}

QString GitRepository::webUrl() const
{
    QString url = m_webUrlOverride.trimmed();
    while (url.endsWith(u'/'))
        url.chop(1);
    return url.isEmpty() ? webUrlFromRemote(m_remoteUrl) : url;
}

void GitRepository::setWebUrlOverride(const QString &url)
{
    if (m_webUrlOverride == url)
        return;
    m_webUrlOverride = url;
    emit remoteChanged();
}

void GitRepository::setBranchFilter(const QString &refName)
{
    if (m_branchFilter == refName)
        return;
    m_branchFilter = refName;
    emit branchFilterChanged();
    if (isValid())
        loadLog();
}

void GitRepository::setMaxCommits(int count)
{
    count = std::max(1, count);
    if (m_maxCommits == count)
        return;
    m_maxCommits = count;
    emit maxCommitsChanged();
    if (isValid())
        loadLog();
}

void GitRepository::open(const QString &pathOrUrl)
{
    QString dir = pathOrUrl;
    if (const QUrl url(pathOrUrl); url.isLocalFile())
        dir = url.toLocalFile();
    dir = QDir(dir).absolutePath();

    if (!QFileInfo(dir).isDir()) {
        setError(tr("%1 is not a directory.").arg(QDir::toNativeSeparators(dir)));
        return;
    }

    ++m_generation;
    runGit(dir, { u"rev-parse"_s, u"--show-toplevel"_s },
           [this, dir](int code, const QByteArray &out, const QByteArray &err) {
               if (code != 0) {
                   const QString reason = QString::fromUtf8(err).trimmed();
                   setError(reason.isEmpty()
                                ? tr("%1 is not a git repository.").arg(QDir::toNativeSeparators(dir))
                                : reason);
                   return;
               }
               const QString top = QDir(QString::fromUtf8(out).trimmed()).absolutePath();
               if (top != m_path) {
                   m_path = top;
                   m_branchFilter.clear();
                   m_details.clear();
                   emit branchFilterChanged();
                   emit detailsChanged();
                   emit pathChanged();
               }
               setError({});
               refresh();
           });
}

void GitRepository::refresh()
{
    if (!isValid())
        return;
    ++m_generation;
    setError({});
    loadRemote();
    loadRefs();
    loadLog();
}

void GitRepository::loadRemote()
{
    const quint64 gen = m_generation;
    runGit(m_path, { u"remote"_s, u"-v"_s }, [this, gen](int code, const QByteArray &out, const QByteArray &) {
        if (gen != m_generation)
            return;
        QString chosen;
        if (code == 0) {
            // Prefer origin; otherwise the first remote listed.
            for (const QString &line : QString::fromUtf8(out).split(u'\n', Qt::SkipEmptyParts)) {
                const QStringList parts = line.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts);
                if (parts.size() < 2)
                    continue;
                if (chosen.isEmpty())
                    chosen = parts.at(1);
                if (parts.at(0) == u"origin"_s) {
                    chosen = parts.at(1);
                    break;
                }
            }
        }
        if (chosen != m_remoteUrl) {
            m_remoteUrl = chosen;
            emit remoteChanged();
        }
    });
}

void GitRepository::loadRefs()
{
    const quint64 gen = m_generation;
    runGit(m_path,
           { u"for-each-ref"_s,
             u"--format=%(refname)%1f%(refname:short)%1f%(objectname)%1f%(committerdate:unix)%1f%(HEAD)%1f%(subject)"_s,
             u"refs/heads"_s, u"refs/remotes"_s, u"refs/tags"_s },
           [this, gen](int code, const QByteArray &out, const QByteArray &err) {
               if (gen != m_generation)
                   return;
               if (code != 0) {
                   setError(QString::fromUtf8(err).trimmed());
                   return;
               }
               parseRefs(out);
           });
}

void GitRepository::parseRefs(const QByteArray &out)
{
    QList<Branch> branches;
    int local = 0;
    int remote = 0;
    int tags = 0;
    QString current;

    for (const QString &line : QString::fromUtf8(out).split(u'\n', Qt::SkipEmptyParts)) {
        const QStringList f = line.split(FieldSep);
        if (f.size() < 6)
            continue;
        const QString &refName = f.at(0);
        if (refName.startsWith(u"refs/tags/"_s)) {
            ++tags;
            continue;
        }
        if (refName.startsWith(u"refs/remotes/"_s) && refName.endsWith(u"/HEAD"_s))
            continue;

        Branch b;
        b.refName = refName;
        b.name = f.at(1);
        b.sha = f.at(2);
        b.date = fromUnix(f.at(3));
        b.isCurrent = f.at(4) == u"*"_s;
        b.subject = f.at(5);
        b.isRemote = refName.startsWith(u"refs/remotes/"_s);
        (b.isRemote ? remote : local)++;
        if (b.isCurrent)
            current = b.name;
        branches.append(std::move(b));
    }

    m_branches.setBranches(std::move(branches));
    m_localBranchCount = local;
    m_remoteBranchCount = remote;
    m_tagCount = tags;
    m_currentBranch = current;
    emit refsChanged();
}

void GitRepository::loadLog()
{
    const quint64 gen = m_generation;
    const quint64 logGen = ++m_logGeneration;
    QStringList args{
        u"log"_s,
        u"--topo-order"_s,
        u"--decorate=full"_s,
        u"--format=%H%x1f%P%x1f%an%x1f%ae%x1f%at%x1f%D%x1f%s%x1e"_s,
        u"--max-count=%1"_s.arg(m_maxCommits),
    };
    if (m_branchFilter.isEmpty())
        args << u"--exclude=refs/stash"_s << u"--all"_s;
    else
        args << m_branchFilter;
    args << u"--"_s;

    runGit(m_path, args, [this, gen, logGen](int code, const QByteArray &out, const QByteArray &err) {
        if (gen != m_generation || logGen != m_logGeneration)
            return;
        if (code != 0) {
            const QString reason = QString::fromUtf8(err).trimmed();
            // An empty repository has no commits to log; that is not an error worth surfacing.
            if (!reason.contains(u"does not have any commits"_s))
                setError(reason);
            parseLog({});
            return;
        }
        parseLog(out);
    });
}

void GitRepository::parseLog(const QByteArray &out)
{
    QList<Commit> commits;
    QSet<QString> contributors;
    QDateTime last;
    QList<int> activity(ActivityWeeks, 0);
    const QDateTime now = QDateTime::currentDateTime();

    const QString text = QString::fromUtf8(out);
    for (const QStringView record : QStringView(text).split(RecordSep)) {
        const QStringView trimmed = record.trimmed();
        if (trimmed.isEmpty())
            continue;
        const QList<QStringView> f = trimmed.split(FieldSep);
        if (f.size() < 7)
            continue;

        Commit c;
        c.sha = f.at(0).toString();
        c.parents = f.at(1).toString().split(u' ', Qt::SkipEmptyParts);
        c.author = f.at(2).toString();
        c.email = f.at(3).toString();
        c.date = fromUnix(f.at(4).toString());
        c.refs = parseDecorations(f.at(5).toString(), &c.isHead);
        c.subject = f.at(6).toString();

        contributors.insert(c.email.toLower());
        if (!last.isValid() || c.date > last)
            last = c.date;
        const qint64 week = c.date.daysTo(now) / 7;
        if (week >= 0 && week < ActivityWeeks)
            ++activity[ActivityWeeks - 1 - int(week)];

        commits.append(std::move(c));
    }

    m_truncated = commits.size() >= m_maxCommits;
    m_contributorCount = int(contributors.size());
    m_lastCommitDate = last;
    m_weeklyActivity = activity;
    m_commits.setCommits(std::move(commits));
    emit statsChanged();
}

void GitRepository::loadDetails(const QString &sha)
{
    const quint64 detailsGen = ++m_detailsGeneration;
    if (sha.isEmpty() || !isValid()) {
        m_details.clear();
        m_detailsLoading = false;
        emit detailsChanged();
        return;
    }
    m_detailsLoading = true;
    emit detailsChanged();

    runGit(m_path,
           { u"show"_s, u"--no-color"_s, u"--diff-merges=first-parent"_s, u"--numstat"_s,
             u"--decorate=full"_s,
             u"--format=%H%x1f%P%x1f%an%x1f%ae%x1f%at%x1f%cn%x1f%ce%x1f%ct%x1f%D%x1f%B%x1e"_s,
             sha, u"--"_s },
           [this, detailsGen](int code, const QByteArray &out, const QByteArray &err) {
               if (detailsGen != m_detailsGeneration)
                   return;
               m_detailsLoading = false;
               if (code != 0) {
                   m_details = { { u"error"_s, QString::fromUtf8(err).trimmed() } };
                   emit detailsChanged();
                   return;
               }
               parseDetails(out);
           });
}

void GitRepository::parseDetails(const QByteArray &out)
{
    const QString text = QString::fromUtf8(out);
    const qsizetype split = text.indexOf(RecordSep);
    const QStringList f = text.left(split).split(FieldSep);
    if (split < 0 || f.size() < 10) {
        m_details = { { u"error"_s, tr("Could not read commit details.") } };
        emit detailsChanged();
        return;
    }

    QVariantList files;
    int added = 0;
    int removed = 0;
    for (const QString &line : text.mid(split + 1).split(u'\n', Qt::SkipEmptyParts)) {
        const QStringList parts = line.split(u'\t');
        if (parts.size() < 3)
            continue;
        const bool binary = parts.at(0) == u"-"_s;
        const int a = parts.at(0).toInt();
        const int r = parts.at(1).toInt();
        added += a;
        removed += r;
        files.append(QVariantMap{
            { u"path"_s, parts.mid(2).join(u'\t') },
            { u"added"_s, a },
            { u"removed"_s, r },
            { u"binary"_s, binary },
        });
    }

    bool isHead = false;
    const QVariantList refs = parseDecorations(f.at(8), &isHead);
    const QString message = f.mid(9).join(FieldSep).trimmed();
    const qsizetype firstBreak = message.indexOf(u'\n');

    m_details = {
        { u"sha"_s, f.at(0) },
        { u"parents"_s, f.at(1).split(u' ', Qt::SkipEmptyParts) },
        { u"author"_s, f.at(2) },
        { u"authorEmail"_s, f.at(3) },
        { u"authorDate"_s, fromUnix(f.at(4)) },
        { u"committer"_s, f.at(5) },
        { u"committerEmail"_s, f.at(6) },
        { u"commitDate"_s, fromUnix(f.at(7)) },
        { u"refs"_s, refs },
        { u"isHead"_s, isHead },
        { u"subject"_s, firstBreak < 0 ? message : message.left(firstBreak) },
        { u"body"_s, firstBreak < 0 ? QString() : message.mid(firstBreak + 1).trimmed() },
        { u"files"_s, files },
        { u"added"_s, added },
        { u"removed"_s, removed },
        { u"url"_s, commitUrl(f.at(0)) },
    };
    emit detailsChanged();
}

QString GitRepository::commitUrl(const QString &sha) const
{
    const QString base = webUrl();
    if (base.isEmpty() || sha.isEmpty())
        return {};
    const QString host = QUrl(base).host().toLower();
    if (host.contains(u"bitbucket"_s))
        return base + u"/commits/"_s + sha;
    if (host.contains(u"gitlab"_s))
        return base + u"/-/commit/"_s + sha;
    return base + u"/commit/"_s + sha;
}

bool GitRepository::openCommitInBrowser(const QString &sha)
{
    const QString url = commitUrl(sha);
    if (url.isEmpty()) {
        emit message(m_remoteUrl.isEmpty()
                         ? tr("This repository has no remote, so there is no web page to open. "
                              "Start visualit with --web-url to set one.")
                         : tr("Cannot derive a web URL from remote %1. Start visualit with --web-url to set one.")
                               .arg(m_remoteUrl));
        return false;
    }
    if (!QDesktopServices::openUrl(QUrl(url))) {
        emit message(tr("Could not open %1").arg(url));
        return false;
    }
    emit message(tr("Opened %1").arg(url));
    return true;
}

QString GitRepository::webUrlFromRemote(const QString &remote)
{
    const QString r = remote.trimmed();
    if (r.isEmpty())
        return {};

    QString scheme = u"https"_s;
    QString host;
    int port = -1;
    QString path;

    if (r.contains(u"://"_s)) {
        const QUrl url(r);
        if (!url.isValid() || url.isLocalFile() || url.host().isEmpty())
            return {};
        host = url.host();
        path = url.path();
        // Keep the port only for web schemes; an ssh port says nothing about the web host.
        if (url.scheme() == u"http"_s || url.scheme() == u"https"_s) {
            scheme = url.scheme();
            port = url.port();
        }
    } else {
        // scp-like syntax: [user@]host:path
        static const QRegularExpression scpLike(u"^(?:[^@/]+@)?([^:/]+):(.+)$"_s);
        const QRegularExpressionMatch m = scpLike.match(r);
        if (!m.hasMatch())
            return {};
        host = m.captured(1);
        path = m.captured(2);
    }

    while (path.startsWith(u'/'))
        path.remove(0, 1);
    while (path.endsWith(u'/'))
        path.chop(1);
    if (path.endsWith(u".git"_s))
        path.chop(4);
    if (path.isEmpty())
        return {};

    // Azure DevOps ssh remotes: ssh.dev.azure.com:v3/org/project/repo
    if (host == u"ssh.dev.azure.com"_s && path.startsWith(u"v3/"_s)) {
        const QStringList parts = path.mid(3).split(u'/');
        if (parts.size() == 3)
            return u"https://dev.azure.com/%1/%2/_git/%3"_s.arg(parts.at(0), parts.at(1), parts.at(2));
        return {};
    }

    QString base = scheme + u"://"_s + host;
    if (port > 0)
        base += u':' + QString::number(port);
    return base + u'/' + path;
}

void GitRepository::runGit(const QString &workingDir, const QStringList &args, GitCallback done)
{
    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(workingDir);
    proc->setProgram(u"git"_s);
    proc->setArguments(QStringList{ u"--no-pager"_s, u"-c"_s, u"color.ui=false"_s,
                                    u"-c"_s, u"core.quotepath=false"_s }
                       + args);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(u"GIT_TERMINAL_PROMPT"_s, u"0"_s);
    env.insert(u"LC_ALL"_s, u"C.UTF-8"_s);
    proc->setProcessEnvironment(env);

    beginTask();
    auto finish = std::make_shared<GitCallback>(std::move(done));
    connect(proc, &QProcess::finished, this, [this, proc, finish](int code, QProcess::ExitStatus status) {
        const QByteArray out = proc->readAllStandardOutput();
        const QByteArray err = proc->readAllStandardError();
        proc->deleteLater();
        (*finish)(status == QProcess::NormalExit ? code : -1, out, err);
        endTask();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc, finish](QProcess::ProcessError e) {
        // FailedToStart is the only error not followed by finished().
        if (e != QProcess::FailedToStart)
            return;
        proc->deleteLater();
        (*finish)(-1, {}, tr("Could not run git: %1").arg(proc->errorString()).toUtf8());
        endTask();
    });
    proc->start();
}

void GitRepository::beginTask()
{
    if (m_pending++ == 0)
        emit loadingChanged();
}

void GitRepository::endTask()
{
    if (--m_pending == 0)
        emit loadingChanged();
}

void GitRepository::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}
