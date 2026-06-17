#pragma once

#include <QString>
#include <QStringList>
#include <memory>

class QClipboard;
class QMimeData;

namespace FileActions
{
struct LaunchPlan
{
    enum class Kind
    {
        Invalid,
        RevealFile,
        Process,
        Url
    };

    Kind kind = Kind::Invalid;
    QString program;
    QStringList arguments;
    QString url;
    QString targetPath;
};

bool isValidLocalFile(const QString& path);
QString sanitizeFileBaseName(const QString& baseName);
QString uniqueFilePath(const QString& directory, const QString& baseName, const QString& extension);
LaunchPlan openFilePlan(const QString& path);
LaunchPlan revealFilePlan(const QString& path);
std::unique_ptr<QMimeData> createFileClipboardMimeData(const QString& path);
bool copyFileToClipboard(const QString& path, QClipboard* clipboard);
bool launch(const LaunchPlan& plan);
}
