#pragma once

#include <QString>
#include <QVariantMap>

#include <optional>

struct ClipMetadata
{
    QString path;
    QString name;
    QString date;
    QString game;
    QString size;
    qint64 size_bytes = 0;
    double duration_sec = 0.0;
    QString duration;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    bool has_audio = false;
};

class ClipProbe
{
public:
    static std::optional<ClipMetadata> probe(const QString& path);
    static QVariantMap toVariantMap(const ClipMetadata& metadata);
    static QString locateProbeExecutable();
    static QString formatDuration(double seconds);
    static QString formatSize(qint64 bytes);
};
