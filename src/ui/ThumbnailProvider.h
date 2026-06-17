#pragma once

#include <QQuickImageProvider>
#include <QPixmap>
#include <unordered_map>
#include <string>

class ThumbnailProvider : public QQuickImageProvider
{
public:
    ThumbnailProvider() : QQuickImageProvider(Pixmap) {}

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    void clearCache() { m_cache.clear(); }

private:
    std::unordered_map<std::string, QPixmap> m_cache;
};
