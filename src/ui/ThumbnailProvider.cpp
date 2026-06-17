#include "ui/ThumbnailProvider.h"
#include "util/Thumbnail.h"

#include <QUrl>

QPixmap ThumbnailProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    QString decoded = QUrl::fromPercentEncoding(id.toUtf8());
    std::string path = decoded.toStdString();

    auto it = m_cache.find(path);
    if (it != m_cache.end())
    {
        if (size) *size = it->second.size();
        return it->second;
    }

    int w = requestedSize.width() > 0 ? requestedSize.width() : 128;
    int h = requestedSize.height() > 0 ? requestedSize.height() : 72;

    QPixmap thumb = ThumbnailExtractor::extract(path, w, h);
    if (!thumb.isNull())
        m_cache[path] = thumb;

    if (size)
        *size = thumb.isNull() ? QSize(w, h) : thumb.size();

    return thumb;
}
