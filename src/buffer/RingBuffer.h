#pragma once

#include <deque>
#include <cstdint>
#include <vector>
#include <mutex>

#include "KeyframeIndex.h"

struct RingBufferPacket
{
    std::vector<uint8_t> data;
    int64_t pts = 0;       // microseconds
    int64_t dts = 0;       // microseconds
    int64_t duration = 0;  // microseconds
    bool is_keyframe = false;
};

// Circular buffer of encoded video packets with keyframe indexing.
// Thread-safe for single-producer (encoder thread) single-consumer (main/UI thread).
class RingBuffer
{
public:
    explicit RingBuffer(int64_t max_duration_us = 5 * 60 * 1'000'000LL);

    // Push an encoded packet onto the buffer.
    // Automatically evicts old packets to maintain the max duration.
    void push(RingBufferPacket packet);

    // Flush a window of packets, starting from the nearest IDR before window_start.
    // out_packets receives all packets in [idr_start, now].
    // max_duration_us: maximum window size; if the buffer has less data,
    //                  the window starts from the earliest available IDR.
    // Returns the actual start PTS used.
    int64_t flushWindow(int64_t window_start, std::vector<RingBufferPacket>& out_packets);

    // Clear all buffered data.
    void clear();

    // Current buffer duration in microseconds (now - earliest packet PTS).
    int64_t duration() const;

    // Number of packets currently buffered.
    size_t packetCount() const;

    // PTS of the most recent packet (0 if buffer is empty).
    int64_t latestPts() const;

private:
    void evictBefore(int64_t pts);

    std::deque<RingBufferPacket> m_packets;
    KeyframeIndex m_keyframe_index;
    int64_t m_max_duration_us;
    int64_t m_byte_offset = 0; // Cumulative byte offset for keyframe index
    mutable std::mutex m_mutex;
};
