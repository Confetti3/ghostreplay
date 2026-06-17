#include "util/FileActions.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QProcess>
#include <QRegularExpression>
#include <QtGlobal>
#include <QUrl>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <shlobj.h>
#endif

namespace FileActions
{
namespace
{
QString absoluteFilePath(const QString& path)
{
    return QFileInfo(path).absoluteFilePath();
}

#if defined(Q_OS_WIN)
bool revealFileInShell(const QString& absolutePath)
{
    const HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);

    PIDLIST_ABSOLUTE item = ILCreateFromPathW(reinterpret_cast<LPCWSTR>(absolutePath.utf16()));
    if (!item)
    {
        if (shouldUninitialize)
            CoUninitialize();
        return false;
    }

    PIDLIST_ABSOLUTE folder = ILClone(item);
    if (!folder)
    {
        CoTaskMemFree(item);
        if (shouldUninitialize)
            CoUninitialize();
        return false;
    }

    if (!ILRemoveLastID(folder))
    {
        CoTaskMemFree(folder);
        CoTaskMemFree(item);
        if (shouldUninitialize)
            CoUninitialize();
        return false;
    }

    PIDLIST_RELATIVE child = ILFindChild(folder, item);
    if (!child)
    {
        CoTaskMemFree(folder);
        CoTaskMemFree(item);
        if (shouldUninitialize)
            CoUninitialize();
        return false;
    }

    PCUITEMID_CHILD childItems[] = { child };
    const HRESULT result = SHOpenFolderAndSelectItems(folder, 1, childItems, 0);

    // ILFindChild returns a borrowed pointer into item, not an allocated PIDL.
    CoTaskMemFree(folder);
    CoTaskMemFree(item);
    if (shouldUninitialize)
        CoUninitialize();
    return SUCCEEDED(result);
}
#endif
}

bool isValidLocalFile(const QString& path)
{
    if (path.trimmed().isEmpty())
        return false;

    const QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile();
}

QString sanitizeFileBaseName(const QString& baseName)
{
    QString sanitized = baseName.trimmed();
    sanitized.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "-");
    sanitized.replace(QRegularExpression("\\s+"), " ");
    sanitized.replace(QRegularExpression("[\\. ]+$"), "");
    if (sanitized.isEmpty())
        sanitized = "clip";
    return sanitized.left(120);
}

QString uniqueFilePath(const QString& directory, const QString& baseName, const QString& extension)
{
    QDir dir(directory.isEmpty() ? QStringLiteral(".") : directory);
    const QString safeBase = sanitizeFileBaseName(baseName);
    const QString safeExtension = extension.startsWith('.') ? extension : "." + extension;

    QString candidate = dir.filePath(safeBase + safeExtension);
    for (int index = 2; QFileInfo::exists(candidate); ++index)
        candidate = dir.filePath(QString("%1 (%2)%3").arg(safeBase).arg(index).arg(safeExtension));

    return candidate;
}

LaunchPlan openFilePlan(const QString& path)
{
    LaunchPlan plan;
    plan.kind = LaunchPlan::Kind::Url;
    plan.url = QUrl::fromLocalFile(absoluteFilePath(path)).toString();
    return plan;
}

LaunchPlan revealFilePlan(const QString& path)
{
    const QFileInfo fileInfo(path);
    const QString absolutePath = fileInfo.absoluteFilePath();

    LaunchPlan plan;
#if defined(Q_OS_WIN)
    plan.kind = LaunchPlan::Kind::RevealFile;
    plan.targetPath = absolutePath;
#elif defined(Q_OS_MACOS)
    plan.kind = LaunchPlan::Kind::Process;
    plan.program = "open";
    plan.arguments = { "-R", absolutePath };
#else
    plan.kind = LaunchPlan::Kind::Url;
    plan.url = QUrl::fromLocalFile(fileInfo.absolutePath()).toString();
#endif
    return plan;
}

std::unique_ptr<QMimeData> createFileClipboardMimeData(const QString& path)
{
    if (!isValidLocalFile(path))
        return {};

    const QString absolutePath = absoluteFilePath(path);
    const QUrl fileUrl = QUrl::fromLocalFile(absolutePath);
    auto mimeData = std::make_unique<QMimeData>();
    mimeData->setUrls({ fileUrl });
    mimeData->setText(absolutePath);
    return mimeData;
}

bool copyFileToClipboard(const QString& path, QClipboard* clipboard)
{
    if (!clipboard)
        return false;

    auto mimeData = createFileClipboardMimeData(path);
    if (!mimeData)
        return false;

    clipboard->setMimeData(mimeData.release());
    return true;
}

bool launch(const LaunchPlan& plan)
{
    if (plan.kind == LaunchPlan::Kind::Invalid)
        return false;

#if defined(Q_OS_WIN)
    if (plan.kind == LaunchPlan::Kind::RevealFile)
    {
        if (revealFileInShell(plan.targetPath))
            return true;

        return QProcess::startDetached(
            "explorer.exe",
            { "/select,", QDir::toNativeSeparators(plan.targetPath) });
    }
#endif

    if (plan.kind == LaunchPlan::Kind::Url)
        return QDesktopServices::openUrl(QUrl(plan.url));

    if (plan.program.isEmpty())
        return false;

    return QProcess::startDetached(plan.program, plan.arguments);
}
}
