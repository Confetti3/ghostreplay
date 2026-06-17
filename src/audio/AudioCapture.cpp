#include "audio/AudioCapture.h"
#include "audio/AudioEncoder.h"
#include "util/Config.h"
#include "util/Logger.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <deque>

#pragma comment(lib, "ole32.lib")

static constexpr int AAC_FRAME_SIZE = 1024;

// ── Internal state ──────────────────────────────────────────────

struct AudioCapture::Impl
{
    AudioEncoder encoder;

    // WASAPI COM interfaces
    IAudioClient*        audio_client  = nullptr;
    IAudioCaptureClient* capture_client = nullptr;

    // Ring buffer of encoded packets
    std::deque<EncodedAudioPacket> buffer;
    std::mutex buffer_mutex;

    // Capture thread
    std::thread       thread;
    std::atomic<bool> running{ false };
    std::atomic<bool> initialized{ false };
    bool              com_initialized = false;

    // PCM accumulator (before AAC frame boundary)
    std::vector<float> pcm_buf;

    // Running counters
    int64_t total_frames = 0;   // total frames captured (for PTS calc)
    int64_t enc_counter  = 0;   // frames submitted to encoder

    AudioConfig config;
};

// ── Constructor / Destructor ────────────────────────────────────

AudioCapture::AudioCapture()
    : m_impl(std::make_unique<Impl>())
{}

AudioCapture::~AudioCapture()
{
    stop();
    if (m_impl->capture_client) m_impl->capture_client->Release();
    if (m_impl->audio_client)   m_impl->audio_client->Release();
    if (m_impl->com_initialized) CoUninitialize();
}

// ── Initialize ──────────────────────────────────────────────────

bool AudioCapture::initialize(const AudioConfig& config)
{
    auto& impl = *m_impl;
    impl.config = config;

    m_sample_rate = config.sample_rate;
    m_channels    = config.channels;

    // ── 1. Init AAC encoder ────────────────────────────────
    if (!impl.encoder.initialize(config.sample_rate,
                                  config.channels,
                                  config.bitrate_kbps))
    {
        LOG_ERROR("AudioCapture: AAC encoder init failed");
        return false;
    }

    // ── 2. Init COM on this thread ─────────────────────────
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        impl.com_initialized = true;
    }
    else if (hr != RPC_E_CHANGED_MODE)
    {
        LOG_ERROR("AudioCapture: CoInitializeEx failed, hr=0x" +
                  std::to_string(hr));
        return false;
    }

    // ── 3. Get default audio render endpoint ───────────────
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                           CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: CoCreateInstance(MMDeviceEnumerator) failed");
        return false;
    }

    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    enumerator->Release();
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: GetDefaultAudioEndpoint failed");
        return false;
    }

    // ── 4. Activate IAudioClient ───────────────────────────
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                           nullptr, (void**)&impl.audio_client);
    device->Release();
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: Activate(IAudioClient) failed");
        return false;
    }

    // ── 5. Get mix format ──────────────────────────────────
    WAVEFORMATEX* pwfx = nullptr;
    hr = impl.audio_client->GetMixFormat(&pwfx);
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: GetMixFormat failed");
        impl.audio_client->Release();
        impl.audio_client = nullptr;
        return false;
    }

    // Convert to extended if needed for channel mask
    WAVEFORMATEXTENSIBLE* pwfxe = nullptr;
    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        pwfxe = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);

    LOG_INFO("AudioCapture: mix format " +
             std::to_string(pwfx->nChannels) + "ch " +
             std::to_string(pwfx->nSamplesPerSec) + "Hz " +
             std::to_string(pwfx->wBitsPerSample) + "bit");

    // ── 6. Initialize in loopback mode ─────────────────────
    REFERENCE_TIME buf_dur = 1'000'000; // 100ms buffer (10ths of 100ns)
    hr = impl.audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        buf_dur,
        0,
        pwfx,
        nullptr);

    CoTaskMemFree(pwfx);

    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: IAudioClient::Initialize failed, hr=0x" +
                  std::to_string(hr));
        impl.audio_client->Release();
        impl.audio_client = nullptr;
        return false;
    }

    // ── 7. Get IAudioCaptureClient ─────────────────────────
    hr = impl.audio_client->GetService(__uuidof(IAudioCaptureClient),
                                        (void**)&impl.capture_client);
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: GetService(IAudioCaptureClient) failed");
        impl.audio_client->Release();
        impl.audio_client = nullptr;
        return false;
    }

    LOG_INFO("AudioCapture: WASAPI loopback ready (" +
             std::to_string(config.sample_rate) + "Hz " +
             std::to_string(config.channels) + "ch)");

    impl.initialized = true;
    return true;
}

// ── Start / Stop ────────────────────────────────────────────────

void AudioCapture::start()
{
    auto& impl = *m_impl;
    if (!impl.initialized || impl.running) return;

    HRESULT hr = impl.audio_client->Start();
    if (FAILED(hr))
    {
        LOG_ERROR("AudioCapture: IAudioClient::Start failed");
        return;
    }

    impl.running = true;
    impl.total_frames = 0;
    impl.enc_counter = 0;
    impl.pcm_buf.clear();

    impl.thread = std::thread(&AudioCapture::captureThread, this);
    LOG_INFO("AudioCapture: capture started");
}

void AudioCapture::stop()
{
    auto& impl = *m_impl;
    impl.running = false;

    if (impl.audio_client)
        impl.audio_client->Stop();

    if (impl.thread.joinable())
        impl.thread.join();

    // Flush remaining encoder frames
    impl.encoder.flush();

    // Drain final packets into ring buffer
    AudioEncoder::Packet ap;
    while (impl.encoder.receive(ap))
    {
        EncodedAudioPacket eap;
        eap.data        = std::move(ap.data);
        eap.pts_samples = ap.pts_samples;
        eap.duration_us = ap.duration_us;

        std::lock_guard<std::mutex> lock(impl.buffer_mutex);
        impl.buffer.push_back(std::move(eap));
    }

    LOG_INFO("AudioCapture: stopped");
}

// ── Capture thread ──────────────────────────────────────────────

void AudioCapture::captureThread()
{
    auto& impl = *m_impl;

    // COM must be initialized on this thread
    HRESULT hr_com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool thread_com_initialized = SUCCEEDED(hr_com);

    while (impl.running)
    {
        Sleep(5); // ~5ms polling interval

        UINT32 packet_size = 0;
        HRESULT hr = impl.capture_client->GetNextPacketSize(&packet_size);
        if (FAILED(hr))
            continue;

        while (packet_size > 0)
        {
            BYTE*  data = nullptr;
            UINT32 num_frames = 0;
            DWORD  flags = 0;

            hr = impl.capture_client->GetBuffer(&data, &num_frames, &flags,
                                                  nullptr, nullptr);
            if (FAILED(hr)) break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data)
            {
                // Convert to float (WASAPI mix format is IEEE float)
                const float* fdata = reinterpret_cast<const float*>(data);
                int total_samples = static_cast<int>(num_frames) * m_channels;

                // Append to accumulator
                impl.pcm_buf.insert(impl.pcm_buf.end(), fdata, fdata + total_samples);

                // Encode complete AAC frames
                int frames_in_buf = static_cast<int>(impl.pcm_buf.size()) / m_channels;
                while (frames_in_buf >= AAC_FRAME_SIZE)
                {
                    int64_t pts = impl.enc_counter;
                    int consume = AAC_FRAME_SIZE * m_channels;

                    impl.encoder.feed(impl.pcm_buf.data(), AAC_FRAME_SIZE, pts);

                    impl.pcm_buf.erase(impl.pcm_buf.begin(),
                                        impl.pcm_buf.begin() + consume);
                    impl.enc_counter += AAC_FRAME_SIZE;
                    frames_in_buf -= AAC_FRAME_SIZE;

                    // Drain encoded packets into ring buffer
                    AudioEncoder::Packet ap;
                    while (impl.encoder.receive(ap))
                    {
                        EncodedAudioPacket eap;
                        eap.data        = std::move(ap.data);
                        eap.pts_samples = ap.pts_samples;
                        eap.duration_us = ap.duration_us;

                        std::lock_guard<std::mutex> lock(impl.buffer_mutex);
                        impl.buffer.push_back(std::move(eap));
                    }
                }
            }

            impl.total_frames += num_frames;
            impl.capture_client->ReleaseBuffer(num_frames);
            impl.capture_client->GetNextPacketSize(&packet_size);
        }
    }

    if (thread_com_initialized)
        CoUninitialize();
}

// ── Ring buffer operations ──────────────────────────────────────

void AudioCapture::flushWindow(int64_t window_start_us,
                                std::vector<EncodedAudioPacket>& out)
{
    auto& impl = *m_impl;

    int64_t window_start_samples = window_start_us * m_sample_rate / 1'000'000LL;

    std::lock_guard<std::mutex> lock(impl.buffer_mutex);

    out.clear();
    for (auto& pkt : impl.buffer)
    {
        if (pkt.pts_samples >= window_start_samples)
            out.push_back(pkt);
    }
}

void AudioCapture::clear()
{
    std::lock_guard<std::mutex> lock(m_impl->buffer_mutex);
    m_impl->buffer.clear();
    m_impl->pcm_buf.clear();
    m_impl->total_frames = 0;
    m_impl->enc_counter  = 0;
}

size_t AudioCapture::packetCount() const
{
    std::lock_guard<std::mutex> lock(m_impl->buffer_mutex);
    return m_impl->buffer.size();
}

std::vector<uint8_t> AudioCapture::codecConfig() const
{
    return m_impl->encoder.codecConfig();
}
