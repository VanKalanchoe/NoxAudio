#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace NoxAudio
{
    class AudioClip;

    enum class AudioEffectType : uint8_t
    {
        EQ = 0,            // four bands: low shelf, two peaks, high shelf
        Filter,            // low / high / band pass
        Compressor,        // feed-forward, soft knee, optional sidechain
        Limiter,           // look-ahead brickwall: nothing passes its ceiling
        Gate,              // closes below a threshold, with hold
        Delay,             // stereo delay: normal / ping pong / cross feedback, damped
        Reverb,            // algorithmic: early reflections + an 8-line feedback delay network
        ConvolutionReverb, // a recorded space: an impulse response clip, partitioned FFT convolution
        Chorus,
        Flanger,
        Phaser,
        Distortion,        // soft / hard clip, foldback, tube; oversampled 2x
        Bitcrusher,
        AcousticReverb,    // Steam Audio's reverb at the listener (baked probes or traced)
        Count
    };

    struct AudioEffectParameterInfo
    {
        std::string Name;
        float Min = 0.0f;
        float Max = 1.0f;
        float Default = 0.0f;
        std::string Unit;
        bool Logarithmic = false;
        bool Toggle = false;
        std::vector<std::string> Options;
    };

    struct AudioEffectInfo
    {
        AudioEffectType Type = AudioEffectType::EQ;
        std::string Name;
        std::string Description;
        std::vector<AudioEffectParameterInfo> Parameters;
        bool Sidechain = false;
        bool ImpulseResponse = false;
        bool ReportsGainReduction = false;
    };

    const AudioEffectInfo& GetAudioEffectInfo(AudioEffectType type);
    std::string_view AudioEffectTypeToString(AudioEffectType type);
    AudioEffectType AudioEffectTypeFromString(std::string_view name);
    int32_t FindAudioEffectParameter(AudioEffectType type, std::string_view name);

    class AudioEffect
    {
    public:
        static constexpr uint32_t MaxFrames = 256;

        virtual ~AudioEffect() = default;
        AudioEffect(const AudioEffect&) = delete;
        AudioEffect& operator=(const AudioEffect&) = delete;

        AudioEffectType GetType() const { return m_Type; }
        void SetParameter(uint32_t index, float value)
        {
            if (index < m_Values.size())
                m_Values[index] = value;
        }
        float GetParameter(uint32_t index) const { return index < m_Values.size() ? m_Values[index] : 0.0f; }

        virtual void Process(float* left, float* right, uint32_t frames, const float* keyLeft = nullptr, const float* keyRight = nullptr) = 0;
        virtual void Reset() = 0;

        float GetGainReductionDb() const { return m_GainReduction.load(std::memory_order_relaxed); }

    protected:
        AudioEffect(AudioEffectType type, float sampleRate);
        float Value(uint32_t index) const { return m_Values[index]; }

        AudioEffectType m_Type;
        float m_SampleRate;
        std::vector<float> m_Values;
        std::atomic<float> m_GainReduction{ 0.0f };
    };

    std::unique_ptr<AudioEffect> CreateAudioEffect(AudioEffectType type, float sampleRate, const AudioClip* impulse = nullptr);
}

namespace Nox
{
    using namespace NoxAudio;
}
