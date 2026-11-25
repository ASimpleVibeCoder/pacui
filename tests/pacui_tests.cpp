#include <QDir>
#include <QFile>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class PacuiTests : public QObject {
    Q_OBJECT

private slots:
    void parsesSearchOutput()
    {
        const QString text =
            "extra/firefox 144.0-1\n"
            "    Standalone web browser\n"
            "    from mozilla.org\n"
            "community/linux-zen 6.12.1-1\n"
            "    Zen kernel\n";
        const QVariantList packages = Backend::parseSearchOutput(text, QSet<QString>{"firefox"});
        QCOMPARE(packages.size(), 2);

        const QVariantMap first = packages.at(0).toMap();
        QCOMPARE(first.value("name").toString(), QString("firefox"));
        QCOMPARE(first.value("repo").toString(), QString("extra"));
        QCOMPARE(first.value("version").toString(), QString("144.0-1"));
        QCOMPARE(first.value("installed").toBool(), true);
        QCOMPARE(first.value("description").toString(), QString("Standalone web browser from mozilla.org"));

        const QVariantMap second = packages.at(1).toMap();
        QCOMPARE(second.value("name").toString(), QString("linux-zen"));
        QCOMPARE(second.value("installed").toBool(), false);
    }

    void ignoresNoiseInSearchOutput()
    {
        const QString text = "no matches found\n:: Synchronizing package databases...\n";
        QCOMPARE(Backend::parseSearchOutput(text, QSet<QString>()), QVariantList());
    }

    void validatesPackageNames()
    {
        QVERIFY(Backend::validPackageName("firefox"));
        QVERIFY(Backend::validPackageName("lib32-foo"));
        QVERIFY(Backend::validPackageName("g++"));
        QVERIFY(!Backend::validPackageName("-rf"));
        QVERIFY(!Backend::validPackageName("foo;bar"));
        QVERIFY(!Backend::validPackageName("foo/bar"));
        QVERIFY(!Backend::validPackageName(""));
    }

    void queryNotifies()
    {
        Backend backend;
        QSignalSpy spy(&backend, &Backend::queryChanged);
        backend.setQuery("vim");
        QCOMPARE(backend.query(), QString("vim"));
        QCOMPARE(spy.count(), 1);
        backend.setQuery("vim");
        QCOMPARE(spy.count(), 1);
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n");
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        write(response, "{\"int\":0}");

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        write(response, "{\"int\":6}");
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(PacuiTests)
#include "pacui_tests.moc"
