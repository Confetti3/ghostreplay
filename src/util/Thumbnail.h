#pragma once

#include <string>
#include <memory>

class QPixmap;

// Extracts a thumbnail frame from an MP4 file using FFmpeg.
// Caches results to avoid re-extraction on every refresh.
class ThumbnailExtractor
{
public:
    ThumbnailExtractor();
    ~ThumbnailExtractor();

    // Extracts one frame from ~1s into the clip, scaled to thumb_w x thumb_h.
    // Returns a valid QPixmap on success, or a null one on failure.
    static QPixmap extract(const std::string& path, int thumb_w = 128, int thumb_h = 72);
};
