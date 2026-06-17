#include <catch2/catch_test_macros.hpp>

#ifdef HAS_QT6
#include "util/FileActions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUrl>
#endif

TEST_CASE("file reveal plan uses the native platform target")
{
#ifndef HAS_QT6
    SKIP("Qt file action helpers require Qt 6");
#else
    const QString path = QDir::fromNativeSeparators("C:/Users/example/Videos/clip.mp4");
    const auto plan = FileActions::revealFilePlan(path);

#if defined(Q_OS_WIN)
    REQUIRE(plan.kind == FileActions::LaunchPlan::Kind::RevealFile);
    REQUIRE(plan.targetPath == QFileInfo(path).absoluteFilePath());
#elif defined(Q_OS_MACOS)
    REQUIRE(plan.kind == FileActions::LaunchPlan::Kind::Process);
    REQUIRE(plan.program == "open");
    REQUIRE(plan.arguments == QStringList({ "-R", QFileInfo(path).absoluteFilePath() }));
#else
    REQUIRE(plan.kind == FileActions::LaunchPlan::Kind::Url);
    REQUIRE(plan.url.startsWith("file:"));
    REQUIRE(plan.url.contains("Videos"));
#endif
#endif
}

TEST_CASE("file open plan targets the exact file URL")
{
#ifndef HAS_QT6
    SKIP("Qt file action helpers require Qt 6");
#else
    const QString path = QDir::fromNativeSeparators("C:/Users/example/Videos/clip with spaces.mp4");
    const auto plan = FileActions::openFilePlan(path);

    REQUIRE(plan.kind == FileActions::LaunchPlan::Kind::Url);
    REQUIRE(plan.url.startsWith("file:"));
    REQUIRE(QUrl(plan.url).toLocalFile().endsWith("clip with spaces.mp4"));
#endif
}

TEST_CASE("local file validation rejects empty missing and directory paths")
{
#ifndef HAS_QT6
    SKIP("Qt file action helpers require Qt 6");
#else
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    QTemporaryFile file(dir.filePath("clipXXXXXX.mp4"));
    REQUIRE(file.open());
    file.close();

    REQUIRE_FALSE(FileActions::isValidLocalFile(QString()));
    REQUIRE_FALSE(FileActions::isValidLocalFile(dir.filePath("missing.mp4")));
    REQUIRE_FALSE(FileActions::isValidLocalFile(dir.path()));
    REQUIRE(FileActions::isValidLocalFile(file.fileName()));
#endif
}

TEST_CASE("clipboard mime data includes file url and path text")
{
#ifndef HAS_QT6
    SKIP("Qt file action helpers require Qt 6");
#else
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    QTemporaryFile file(dir.filePath("clipXXXXXX.mp4"));
    REQUIRE(file.open());
    file.close();

    auto mimeData = FileActions::createFileClipboardMimeData(file.fileName());
    REQUIRE(mimeData);
    REQUIRE(mimeData->urls().size() == 1);
    REQUIRE(mimeData->urls().front().toLocalFile() == QFileInfo(file.fileName()).absoluteFilePath());
    REQUIRE(mimeData->text() == QFileInfo(file.fileName()).absoluteFilePath());

    auto missingMimeData = FileActions::createFileClipboardMimeData(dir.filePath("missing.mp4"));
    REQUIRE_FALSE(missingMimeData);
#endif
}

TEST_CASE("unique file path sanitizes names and preserves existing exports")
{
#ifndef HAS_QT6
    SKIP("Qt file action helpers require Qt 6");
#else
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    const QString first = FileActions::uniqueFilePath(dir.path(), "Clip:Name?", ".mp4");
    REQUIRE(QFileInfo(first).fileName() == "Clip-Name-.mp4");

    QFile existing(first);
    REQUIRE(existing.open(QIODevice::WriteOnly));
    existing.close();

    const QString second = FileActions::uniqueFilePath(dir.path(), "Clip:Name?", "mp4");
    REQUIRE(QFileInfo(second).fileName() == "Clip-Name- (2).mp4");
#endif
}
