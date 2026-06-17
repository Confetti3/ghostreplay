#include "util/ClipProbe.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtMath>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

namespace
{
struct CachedClipMetadata
{
    qint64 last_modified_ms = 0;
    qint64 size_bytes = 0;
    ClipMetadata metadata;
};

QHash<QString, CachedClipMetadata> g_metadata_cache;
bool g_metadata_cache_loaded = false;

double parseFrameRate(const QString& value)
{
    const QStringList parts = value.split('/');
    if (parts.size() == 2)
    {
        bool num_ok = false;
        bool den_ok = false;
        const double num = parts[0].toDouble(&num_ok);
        const double den = parts[1].toDouble(&den_ok);
        if (num_ok && den_ok && den > 0.0)
            return num / den;
    }

    bool ok = false;
    const double direct = value.toDouble(&ok);
    return ok ? direct : 0.0;
}

QString inferGameName(const QFileInfo& file_info)
{
    const QString base = file_info.completeBaseName();
    const QRegularExpression re("^(.+)_(\\d{4}-\\d{2}-\\d{2}_\\d{2}-\\d{2}-\\d{2})$");
    const auto match = re.match(base);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString metadataCachePath()
{
    const QString cache_root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString base_dir = cache_root.isEmpty()
        ? QDir::temp().filePath("GhostReplay")
        : cache_root;
    QDir().mkpath(base_dir);
    return QDir(base_dir).filePath("clip-metadata-cache.json");
}

QJsonObject metadataToJson(const ClipMetadata& metadata)
{
    QJsonObject obj;
    obj["path"] = metadata.path;
    obj["name"] = metadata.name;
    obj["date"] = metadata.date;
    obj["game"] = metadata.game;
    obj["size"] = metadata.size;
    obj["sizeBytes"] = QString::number(metadata.size_bytes);
    obj["durationSec"] = metadata.duration_sec;
    obj["duration"] = metadata.duration;
    obj["width"] = metadata.width;
    obj["height"] = metadata.height;
    obj["fps"] = metadata.fps;
    obj["hasAudio"] = metadata.has_audio;
    return obj;
}

ClipMetadata metadataFromJson(const QJsonObject& obj)
{
    ClipMetadata metadata;
    metadata.path = obj.value("path").toString();
    metadata.name = obj.value("name").toString();
    metadata.date = obj.value("date").toString();
    metadata.game = obj.value("game").toString();
    metadata.size = obj.value("size").toString();
    metadata.size_bytes = obj.value("sizeBytes").toString().toLongLong();
    metadata.duration_sec = obj.value("durationSec").toDouble();
    metadata.duration = obj.value("duration").toString();
    metadata.width = obj.value("width").toInt();
    metadata.height = obj.value("height").toInt();
    metadata.fps = obj.value("fps").toDouble();
    metadata.has_audio = obj.value("hasAudio").toBool();
    return metadata;
}

void loadMetadataCache()
{
    if (g_metadata_cache_loaded)
        return;
    g_metadata_cache_loaded = true;

    QFile file(metadataCachePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = doc.object();
    const QJsonObject clips = root.value("clips").toObject();

    for (auto it = clips.begin(); it != clips.end(); ++it)
    {
        const QJsonObject obj = it.value().toObject();
        CachedClipMetadata cached;
        cached.last_modified_ms = obj.value("lastModifiedMs").toVariant().toLongLong();
        cached.size_bytes = obj.value("fileSizeBytes").toVariant().toLongLong();
        cached.metadata = metadataFromJson(obj.value("metadata").toObject());
        if (!cached.metadata.path.isEmpty())
            g_metadata_cache.insert(it.key(), cached);
    }
}

void saveMetadataCache()
{
    QJsonObject clips;
    for (auto it = g_metadata_cache.constBegin(); it != g_metadata_cache.constEnd(); ++it)
    {
        QJsonObject obj;
        obj["lastModifiedMs"] = QString::number(it->last_modified_ms);
        obj["fileSizeBytes"] = QString::number(it->size_bytes);
        obj["metadata"] = metadataToJson(it->metadata);
        clips[it.key()] = obj;
    }

    QJsonObject root;
    root["version"] = 1;
    root["clips"] = clips;

    QFile file(metadataCachePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void hideProcessWindow(QProcess& process)
{
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags &= ~CREATE_NEW_CONSOLE;
        args->flags |= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
    });
#else
    Q_UNUSED(process);
#endif
}
}

QString ClipProbe::locateProbeExecutable()
{
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QString local = QDir(app_dir).filePath("ffprobe.exe");
    if (QFileInfo::exists(local))
        return local;

    const QString in_path = QStandardPaths::findExecutable("ffprobe");
    if (!in_path.isEmpty())
        return in_path;

    const QString env_root = qEnvironmentVariable("FFMPEG_DIR");
    if (!env_root.isEmpty())
    {
        const QString env_path = QDir(env_root).filePath("bin/ffprobe.exe");
        if (QFileInfo::exists(env_path))
            return env_path;
    }

    return QString();
}

std::optional<ClipMetadata> ClipProbe::probe(const QString& path)
{
    QFileInfo file_info(path);
    if (!file_info.exists())
        return std::nullopt;

    const QString absolute_path = file_info.absoluteFilePath();
    const qint64 last_modified_ms = file_info.lastModified().toMSecsSinceEpoch();
    const qint64 file_size = file_info.size();

    loadMetadataCache();
    const auto cached = g_metadata_cache.constFind(absolute_path);
    if (cached != g_metadata_cache.constEnd() &&
        cached->last_modified_ms == last_modified_ms &&
        cached->size_bytes == file_size)
    {
        return cached->metadata;
    }

    ClipMetadata metadata;
    metadata.path = absolute_path;
    metadata.name = file_info.fileName();
    metadata.date = file_info.lastModified().toString("yyyy-MM-dd hh:mm:ss");
    metadata.game = inferGameName(file_info);
    metadata.size_bytes = file_size;
    metadata.size = formatSize(metadata.size_bytes);

    const QString ffprobe = locateProbeExecutable();
    if (ffprobe.isEmpty())
    {
        g_metadata_cache[absolute_path] = { last_modified_ms, file_size, metadata };
        saveMetadataCache();
        return metadata;
    }

    QProcess probe;
    hideProcessWindow(probe);
    QStringList args{
        "-v", "error",
        "-show_entries", "format=duration,size:stream=codec_type,width,height,avg_frame_rate,r_frame_rate",
        "-of", "json",
        path
    };
    probe.start(ffprobe, args);
    if (!probe.waitForFinished(12000) || probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0)
    {
        g_metadata_cache[absolute_path] = { last_modified_ms, file_size, metadata };
        saveMetadataCache();
        return metadata;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(probe.readAllStandardOutput());
    if (!doc.isObject())
    {
        g_metadata_cache[absolute_path] = { last_modified_ms, file_size, metadata };
        saveMetadataCache();
        return metadata;
    }

    const QJsonObject root = doc.object();
    const QJsonObject format = root.value("format").toObject();
    metadata.duration_sec = format.value("duration").toString().toDouble();

    const QJsonArray streams = root.value("streams").toArray();
    for (const auto& stream_value : streams)
    {
        const QJsonObject stream = stream_value.toObject();
        const QString codec_type = stream.value("codec_type").toString();
        if (codec_type == "video" && metadata.width == 0)
        {
            metadata.width = stream.value("width").toInt();
            metadata.height = stream.value("height").toInt();
            metadata.fps = parseFrameRate(stream.value("avg_frame_rate").toString());
            if (metadata.fps <= 0.0)
                metadata.fps = parseFrameRate(stream.value("r_frame_rate").toString());
        }
        else if (codec_type == "audio")
        {
            metadata.has_audio = true;
        }
    }

    metadata.duration = formatDuration(metadata.duration_sec);
    g_metadata_cache[absolute_path] = { last_modified_ms, file_size, metadata };
    saveMetadataCache();
    return metadata;
}

QVariantMap ClipProbe::toVariantMap(const ClipMetadata& metadata)
{
    QVariantMap entry;
    entry["path"] = metadata.path;
    entry["name"] = metadata.name;
    entry["date"] = metadata.date;
    entry["game"] = metadata.game;
    entry["size"] = metadata.size;
    entry["sizeBytes"] = metadata.size_bytes;
    entry["duration"] = metadata.duration;
    entry["durationSec"] = metadata.duration_sec;
    entry["width"] = metadata.width;
    entry["height"] = metadata.height;
    entry["fps"] = metadata.fps;
    entry["hasAudio"] = metadata.has_audio;
    entry["resolution"] = metadata.width > 0 && metadata.height > 0
        ? QString("%1x%2").arg(metadata.width).arg(metadata.height)
        : QString();
    return entry;
}

QString ClipProbe::formatDuration(double seconds)
{
    if (seconds <= 0.0)
        return QString();

    const int rounded = qRound(seconds);
    const int hours = rounded / 3600;
    const int minutes = (rounded % 3600) / 60;
    const int secs = rounded % 60;

    if (hours > 0)
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));

    return QString("%1:%2")
        .arg(minutes)
        .arg(secs, 2, 10, QChar('0'));
}

QString ClipProbe::formatSize(qint64 bytes)
{
    if (bytes <= 0)
        return QString("0 MB");

    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    if (mb >= 1024.0)
        return QString("%1 GB").arg(mb / 1024.0, 0, 'f', 2);
    return QString("%1 MB").arg(mb, 0, 'f', mb >= 100.0 ? 0 : 1);
}
