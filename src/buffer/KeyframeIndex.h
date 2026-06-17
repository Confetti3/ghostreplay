#pragma once

#include <cstdint>
#include <vector>

// Tracks IDR (keyframe) positions within the ring buffer.
// Enables clean clip starts — clips always begin on an IDR frame.
class KeyframeIndex
{
public:
    struct Entry
    {
        int64_t pts;       // Presentation timestamp of the IDR frame, microseconds
        int64_t byte_offset; // Offset in bytes from the ring buffer start
        int64_t dts;       // Decode timestamp, microseconds
    };

    // Record an IDR frame position.
    void add(int64_t pts, int64_t dts, int64_t byte_offset);

    // Find the nearest IDR at or before the given PTS.
    // Returns false if no IDR exists (buffer underrun).
    bool findNearestBefore(int64_t pts, Entry& out) const;

    // Find the nearest IDR at or after the given PTS.
    bool findNearestAfter(int64_t pts, Entry& out) const;

    // Remove entries with PTS less than the given threshold.
    // Used when old packets are evicted from the ring buffer.
    void evictBefore(int64_t pts);

    // Number of IDR entries tracked.
    size_t size() const { return m_entries.size(); }

    // Clear all entries.
    void clear() { m_entries.clear(); }

    // Get raw entries (for export/cloning).
    const std::vector<Entry>& entries() const { return m_entries; }

private:
    std::vector<Entry> m_entries; // Sorted by PTS (ascending)
};
