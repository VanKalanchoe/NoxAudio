#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "AudioRef.h"
#include "AudioTypes.h"

namespace NoxAudio
{
    // A sound file as an asset: .wav / .ogg / .flac / .mp3, or created directly from in-memory float PCM samples.
    class AudioClip : public Asset
    {
    public:
        static AssetType GetStaticType() { return AssetType::AudioClip; }
        AssetType GetType() const override { return GetStaticType(); }

        // Reads `path` through the FileReadCallback (defaults to std::ifstream). Null when it is not a sound miniaudio decodes.
        static Ref<AudioClip> Load(const std::filesystem::path& path, AudioLoadType loadType = AudioLoadType::Auto);

        // Creates a decoded AudioClip directly from interleaved float PCM samples in memory.
        static Ref<AudioClip> CreateFromSamples(std::vector<float> samples, uint32_t sampleRate, uint32_t channels = 1,
                                                std::filesystem::path path = "procedural.wav");

        const std::filesystem::path& GetPath() const { return m_Path; }
        uint32_t GetSampleRate() const { return m_SampleRate; }
        uint32_t GetChannels() const { return m_Channels; } // 1 or 2 (more are downmixed to stereo)
        uint64_t GetFrameCount() const { return m_FrameCount; }
        float GetDuration() const { return m_SampleRate ? static_cast<float>(static_cast<double>(m_FrameCount) / m_SampleRate) : 0.0f; }
        // Never Auto: what Auto chose for this clip.
        AudioLoadType GetLoadType() const { return m_LoadType; }
        bool IsDecoded() const { return m_LoadType == AudioLoadType::DecompressOnLoad; }

        // Decompress On Load: the samples, interleaved floats (GetFrameCount() * GetChannels()).
        const std::vector<float>& GetSamples() const { return m_Samples; }
        // Compressed In Memory: the file's bytes.
        const std::vector<uint8_t>& GetEncodedBytes() const { return m_Encoded; }
        // Compressed In Memory / Streaming: the first second decoded (stream priming).
        const std::vector<float>& GetPrimeSamples() const { return m_Prime; }
        uint64_t GetPrimeFrames() const { return m_Channels ? m_Prime.size() / m_Channels : 0; }
        size_t GetMemoryBytes() const { return (m_Samples.size() + m_Prime.size()) * sizeof(float) + m_Encoded.size(); }

    private:
        void LoadPrime();

        std::filesystem::path m_Path;
        uint32_t m_SampleRate = 0;
        uint32_t m_Channels = 0;
        uint64_t m_FrameCount = 0;
        AudioLoadType m_LoadType = AudioLoadType::DecompressOnLoad;
        std::vector<float> m_Samples;
        std::vector<uint8_t> m_Encoded;
        std::vector<float> m_Prime;
    };

    // Decodes a Compressed In Memory or Streaming clip as it plays (AudioEngine's stream thread, one per playing voice).
    class AudioClipDecoder
    {
    public:
        AudioClipDecoder();
        ~AudioClipDecoder();
        AudioClipDecoder(const AudioClipDecoder&) = delete;
        AudioClipDecoder& operator=(const AudioClipDecoder&) = delete;

        bool Open(const Ref<AudioClip>& clip);
        uint64_t Read(float* out, uint64_t frames);
        bool Seek(uint64_t frame);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    // A Compressed In Memory or Streaming clip read from any frame, for an audio graph's Wave Player.
    class AudioClipStream
    {
    public:
        explicit AudioClipStream(uint32_t ringFrames);
        ~AudioClipStream();
        AudioClipStream(const AudioClipStream&) = delete;
        AudioClipStream& operator=(const AudioClipStream&) = delete;

        void Request(const AudioClip* clip, uint64_t frame);
        bool IsReady() const;
        uint32_t Read(float* stereo, uint32_t frames);
        bool HasEnded() const;
        const AudioClip* GetRequestedClip() const { return m_ConsumerClip; }
        uint64_t GetRequestedFrame() const { return m_ConsumerFrame; }

        void Fill();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
        const AudioClip* m_ConsumerClip = nullptr;
        uint64_t m_ConsumerFrame = 0;
        uint32_t m_ConsumerGeneration = 0;
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
