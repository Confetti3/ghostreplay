#include "buffer/RingBuffer.h"
#include "util/Logger.h"

RingBuffer::RingBuffer(int64_t max_duration_us)
    : m_max_duration_us(max_duration_us)
{
}

void RingBuffer::push(RingBufferPacket packet)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_packets.push_back(std::move(packet));
    const auto& pkt = m_packets.back();

    // Track keyframes
    if (pkt.is_keyframe)
    {
        m_keyframe_index.add(pkt.pts, pkt.dts, m_byte_offset);
    }
    m_byte_offset += pkt.data.size();

    // Evict old packets to stay within duration limit
    if (!m_packets.empty())
    {
        int64_t oldest_pts = m_packets.front().pts;
        int64_t cutoff_pts = pkt.pts - m_max_duration_us;

        if (oldest_pts < cutoff_pts)
        {
            evictBefore(cutoff_pts);
        }
    }
}

void RingBuffer::evictBefore(int64_t pts)
{
    // Remove packets with PTS < cutoff
    while (!m_packets.empty() && m_packets.front().pts < pts)
    {
        m_packets.pop_front();
    }

    // Remove stale keyframe entries
    m_keyframe_index.evictBefore(pts);
}

int64_t RingBuffer::flushWindow(int64_t window_start,
                                 std::vector<RingBufferPacket>& out_packets)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    out_packets.clear();

    if (m_packets.empty())
    {
        LOG_WARNING("RingBuffer: flushWindow called on empty buffer");
        return 0;
    }

    // Find the IDR frame at or before window_start
    KeyframeIndex::Entry idr_entry;
    int64_t idr_pts = 0;
    bool found_idr = m_keyframe_index.findNearestBefore(window_start, idr_entry);

    if (!found_idr)
    {
        // No IDR before window_start — use the earliest IDR in the buffer
        LOG_WARNING("RingBuffer: no IDR before window_start, using earliest IDR");
        if (!m_keyframe_index.findNearestAfter(0, idr_entry))
        {
            LOG_ERROR("RingBuffer: no IDR frames in buffer at all");
            return 0;
        }
    }
    idr_pts = idr_entry.pts;

    // Collect all packets from the IDR to end of buffer
    bool collecting = false;
    for (const auto& pkt : m_packets)
    {
        if (!collecting && pkt.pts >= idr_pts)
            collecting = true;

        if (collecting)
        {
            out_packets.push_back(pkt);
        }
    }

    LOG_INFO("RingBuffer: flushed " + std::to_string(out_packets.size()) +
             " packets, IDR at " + std::to_string(idr_pts) + " us");

    return idr_pts;
}

void RingBuffer::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_packets.clear();
    m_keyframe_index.clear();
    m_byte_offset = 0;
}

int64_t RingBuffer::duration() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_packets.empty()) return 0;
    return m_packets.back().pts - m_packets.front().pts;
}

size_t RingBuffer::packetCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_packets.size();
}

int64_t RingBuffer::latestPts() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_packets.empty()) return 0;
    return m_packets.back().pts;
}
