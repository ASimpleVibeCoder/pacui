#include "backend.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>

Backend::Backend(QObject *parent)
    : QObject(parent), m_process(new QProcess(this)) {
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            setBusy(false);
            setStatus(tr("Could not start %1.").arg(m_program));
        }
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const QString text = QString::fromUtf8(m_process->readAllStandardOutput());
        if (m_command == Command::Search)
            m_searchBuffer += text;
        else
            appendOutput(text);
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        const QString text = QString::fromUtf8(m_process->readAllStandardError());
        if (m_command == Command::Search)
            m_searchBuffer += text;
        else
            appendOutput(text);
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Backend::onFinished);
}

Backend::~Backend() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void Backend::setQuery(const QString &query) {
    if (m_query == query)
        return;
    m_query = query;
    emit queryChanged();
}

void Backend::setStatus(const QString &status) {
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void Backend::setBusy(bool busy) {
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void Backend::appendOutput(const QString &text) {
    if (text.isEmpty())
        return;
    m_output += text;
    emit outputChanged();
}

bool Backend::canElevate() const {
    return !QStandardPaths::findExecutable("pkexec").isEmpty();
}

bool Backend::validPackageName(const QString &name) {
    if (name.isEmpty())
        return false;
    static const QRegularExpression pattern("^[A-Za-z0-9][A-Za-z0-9@._+\\-]*$");
    return pattern.match(name).hasMatch();
}

QVariantList Backend::parseSearchOutput(const QString &text, const QSet<QString> &installed) {
    QVariantList result;
    QVariantMap current;

    auto flush = [&]() {
        if (current.isEmpty())
            return;
        result.append(current);
        current.clear();
    };

    const auto lines = text.split('\n', Qt::SkipEmptyParts);
    for (const auto &raw : lines) {
        if (!raw.isEmpty() && (raw.at(0) == ' ' || raw.at(0) == '\t')) {
            if (current.isEmpty())
                continue;
            const QString line = raw.trimmed();
            if (line.isEmpty())
                continue;
            const QString previous = current.value("description").toString();
            current.insert("description", previous.isEmpty() ? line : previous + " " + line);
            continue;
        }

        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;
        const auto parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            flush();
            continue;
        }
        const QString id = parts.at(0);
        const int slash = id.indexOf('/');
        if (slash <= 0) {
            flush();
            continue;
        }
        const QString repo = id.left(slash);
        const QString name = id.mid(slash + 1);
        if (!validPackageName(name)) {
            flush();
            continue;
        }
        flush();
        current.insert("name", name);
        current.insert("repo", repo);
        current.insert("version", parts.at(1));
        current.insert("installed", installed.contains(name));
        current.insert("description", QString());
    }
    flush();
    return result;
}

void Backend::loadInstalled() {
    m_installed.clear();
    QProcess process;
    process.start("pacman", {"-Q"});
    if (!process.waitForFinished(5000) || process.exitCode() != 0)
        return;
    const auto lines = QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const auto parts = line.split(' ', Qt::SkipEmptyParts);
        if (!parts.isEmpty())
            m_installed.insert(parts.first());
    }
}

void Backend::refreshInstalledFlags() {
    for (int i = 0; i < m_packages.size(); ++i) {
        QVariantMap item = m_packages.at(i).toMap();
        item.insert("installed", m_installed.contains(item.value("name").toString()));
        m_packages[i] = item;
    }
    emit packagesChanged();
}

void Backend::run(Command command, const QString &program, const QStringList &args,
                  const QString &message, const QString &package) {
    if (m_busy) {
        setStatus(tr("Another operation is already running."));
        return;
    }
    m_command = command;
    m_program = program;
    m_package = package;
    m_searchBuffer.clear();
    setBusy(true);
    setStatus(message);
    if (command != Command::Search)
        appendOutput("$ " + program + " " + args.join(' ') + "\n");
    m_process->start(program, args);
}

void Backend::search() {
    const QString term = m_query.trimmed();
    if (term.isEmpty()) {
        m_packages.clear();
        emit packagesChanged();
        setStatus(tr("Type a package name to search."));
        return;
    }
    static const QRegularExpression pattern("^[A-Za-z0-9*][A-Za-z0-9@._+\\-*]*$");
    if (!pattern.match(term).hasMatch()) {
        setStatus(tr("Use letters, digits and - + . _ @ only."));
        return;
    }
    loadInstalled();
    run(Command::Search, "pacman", {"-Ss", term}, tr("Searching for %1...").arg(term));
}

void Backend::install(const QString &package) {
    if (!canElevate()) {
        setStatus(tr("pkexec is not installed."));
        return;
    }
    if (!validPackageName(package)) {
        setStatus(tr("Invalid package name."));
        return;
    }
    run(Command::Install, "pkexec", {"pacman", "-S", "--noconfirm", package},
        tr("Installing %1...").arg(package), package);
}

void Backend::remove(const QString &package) {
    if (!canElevate()) {
        setStatus(tr("pkexec is not installed."));
        return;
    }
    if (!validPackageName(package)) {
        setStatus(tr("Invalid package name."));
        return;
    }
    run(Command::Remove, "pkexec", {"pacman", "-R", "--noconfirm", package},
        tr("Removing %1...").arg(package), package);
}

void Backend::upgrade() {
    if (!canElevate()) {
        setStatus(tr("pkexec is not installed."));
        return;
    }
    run(Command::Upgrade, "pkexec", {"pacman", "-Syu", "--noconfirm"}, tr("Upgrading the system..."));
}

void Backend::refreshInstalled() {
    loadInstalled();
    refreshInstalledFlags();
    setStatus(tr("Installed packages refreshed."));
}

void Backend::clearOutput() {
    m_output.clear();
    emit outputChanged();
}

void Backend::onFinished(int exitCode, QProcess::ExitStatus status) {
    const bool ok = status == QProcess::NormalExit && exitCode == 0;
    const Command command = m_command;
    const QString package = m_package;
    setBusy(false);

    if (command == Command::Search) {
        if (ok) {
            m_packages = parseSearchOutput(m_searchBuffer, m_installed);
            emit packagesChanged();
            setStatus(tr("Found %1 packages.").arg(m_packages.size()));
        } else {
            m_packages.clear();
            emit packagesChanged();
            setStatus(m_searchBuffer.contains("no matches found", Qt::CaseInsensitive)
                          ? tr("No packages found.")
                          : tr("Search failed. Check the output below."));
        }
        m_searchBuffer.clear();
        return;
    }

    if (command == Command::Install || command == Command::Remove) {
        if (ok) {
            loadInstalled();
            refreshInstalledFlags();
            setStatus(command == Command::Install ? tr("Installed %1.").arg(package)
                                                  : tr("Removed %1.").arg(package));
        } else {
            setStatus(command == Command::Install ? tr("Could not install %1.").arg(package)
                                                  : tr("Could not remove %1.").arg(package));
        }
        return;
    }

    setStatus(ok ? tr("System upgraded.") : tr("Upgrade failed. Check the output below."));
    if (ok)
        loadInstalled();
}
