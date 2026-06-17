#include "buffer/KeyframeIndex.h"
#include <algorithm>

void KeyframeIndex::add(int64_t pts, int64_t dts, int64_t byte_offset)
{
    Entry entry{ pts, byte_offset, dts };

    // Keep sorted: find insertion point
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), pts,
        [](const Entry& e, int64_t p) { return e.pts < p; });

    m_entries.insert(it, entry);
}

bool KeyframeIndex::findNearestBefore(int64_t pts, Entry& out) const
{
    if (m_entries.empty()) return false;

    // Binary search for last entry with PTS <= pts
    auto it = std::upper_bound(m_entries.begin(), m_entries.end(), pts,
        [](int64_t p, const Entry& e) { return p < e.pts; });

    if (it == m_entries.begin()) return false;

    out = *(it - 1);
    return true;
}

bool KeyframeIndex::findNearestAfter(int64_t pts, Entry& out) const
{
    if (m_entries.empty()) return false;

    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), pts,
        [](const Entry& e, int64_t p) { return e.pts < p; });

    if (it == m_entries.end()) return false;

    out = *it;
    return true;
}

void KeyframeIndex::evictBefore(int64_t pts)
{
    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), pts,
        [](const Entry& e, int64_t p) { return e.pts < p; });

    m_entries.erase(m_entries.begin(), it);
}
