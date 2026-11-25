#pragma once

#include <QObject>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QVariantList>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QVariantList packages READ packages NOTIFY packagesChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool canElevate READ canElevate CONSTANT)
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(bool omarchy READ isOmarchy CONSTANT)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    QString query() const { return m_query; }
    void setQuery(const QString &query);

    QVariantList packages() const { return m_packages; }
    QString status() const { return m_status; }
    QString output() const { return m_output; }
    bool busy() const { return m_busy; }
    bool canElevate() const;
    bool supported() const { return isArchBased(); }

    static bool isArchBased(const QString &osReleasePath = "/etc/os-release");
    static bool isOmarchy();
    static bool validPackageName(const QString &name);
    static QVariantList parseSearchOutput(const QString &text, const QSet<QString> &installed);

    Q_INVOKABLE void search();
    Q_INVOKABLE void install(const QString &package);
    Q_INVOKABLE void remove(const QString &package);
    Q_INVOKABLE void upgrade();
    Q_INVOKABLE void refreshInstalled();
    Q_INVOKABLE void clearOutput();

signals:
    void queryChanged();
    void packagesChanged();
    void statusChanged();
    void outputChanged();
    void busyChanged();

private:
    enum class Command { Search, Install, Remove, Upgrade };

    void run(Command command, const QString &program, const QStringList &args,
             const QString &message, const QString &package = {});
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void loadInstalled();
    void refreshInstalledFlags();
    void setStatus(const QString &status);
    void setBusy(bool busy);
    void appendOutput(const QString &text);

    QString m_query;
    QVariantList m_packages;
    QSet<QString> m_installed;
    QString m_status;
    QString m_output;
    QString m_searchBuffer;
    QString m_package;
    QString m_program;
    bool m_busy = false;
    Command m_command = Command::Search;
    QProcess *m_process = nullptr;
};
