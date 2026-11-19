#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AudioClip.h"
#include "AudioEffects.h"
#include "AudioTypes.h"
#include "NodeGraphTypes.h"

namespace NoxAudio
{
    class AudioGraphProgram;

    struct AudioSpatialSettings
    {
        float MinDistance = 1.0f;
        float MaxDistance = 50.0f;
        AudioAttenuation Attenuation = AudioAttenuation::Inverse;
        float DopplerLevel = 1.0f;
        AudioSpatialization Spatialization = AudioSpatialization::Default;
        bool AirAbsorption = true;
        bool Occlusion = true;
        float SourceRadius = 0.5f;
        bool Reflections = true;
        bool Pathing = false;
    };

    struct AudioReflectionSettings
    {
        bool Enabled = true;
        int32_t Rays = 4096;
        int32_t Bounces = 4;
        float Duration = 1.0f;
        int32_t MaxSources = 4;
        uint64_t Bus = kAudioBusMaster;
        float LevelDb = 0.0f;
        AudioListenerReverb ListenerReverb = AudioListenerReverb::Baked;
        int32_t PathingSources = 8;
        bool operator==(const AudioReflectionSettings&) const = default;
    };

    struct AudioVoiceEnvironment
    {
        float Volume = 1.0f;
        float LowPassHz = 20000.0f;
        uint64_t SendBus = 0;
        float SendLevel = 0.0f;
        bool operator==(const AudioVoiceEnvironment&) const = default;
    };

    struct AudioPlayDesc
    {
        Ref<AudioClip> Clip;
        std::shared_ptr<const AudioGraphProgram> Graph;
        std::unordered_map<std::string, NodeGraphValue> GraphParameters;
        float Volume = 1.0f;
        float Pitch = 1.0f;
        bool Loop = false;
        uint64_t Bus = kAudioBusSFX;
        AudioVoiceGroup Group = AudioVoiceGroup::Game;
        bool Spatial = false;
        glm::dvec3 Position{ 0.0 };
        AudioSpatialSettings Spatialization;
        float StartTime = 0.0f;
        float FadeIn = 0.0f;
        AudioVoiceEnvironment Environment;
        uint64_t SoundAsset = 0;
        AudioSoundSettings Sound;
    };

    struct AudioStats
    {
        bool DeviceOpen = false;
        std::string DeviceName;
        uint32_t SampleRate = 0;
        uint32_t PeriodFrames = 0;
        uint32_t Voices = 0;
        uint32_t RealVoices = 0;
        uint32_t VirtualVoices = 0;
        uint32_t MaxRealVoices = 0;
        uint32_t StreamingVoices = 0;
        uint64_t Underruns = 0;
        uint64_t StreamStarvations = 0;
        float MixBlockMilliseconds = 0.0f;
        float MixBlockMaxMilliseconds = 0.0f;
        float BlockMilliseconds = 0.0f;
    };

    struct AudioBusMeter
    {
        float PeakLeft = 0.0f, PeakRight = 0.0f;
        float RmsLeft = 0.0f, RmsRight = 0.0f;
        float GainReductionDb = 0.0f;
    };

    struct AudioMixerLayoutDesc
    {
        struct Effect
        {
            uint64_t Id = 0;
            AudioEffectType Type = AudioEffectType::EQ;
            Ref<AudioClip> Impulse;
            std::vector<float> Values;
            bool Bypass = false;
            int32_t Sidechain = -1;
        };
        struct Send
        {
            int32_t Target = -1;
            float Gain = 0.0f;
            bool PreFader = false;
        };
        struct Bus
        {
            uint64_t Id = 0;
            int32_t Parent = -1;
            float Gain = 1.0f;
            bool PlaysWhilePaused = false;
            std::vector<Effect> Effects;
            std::vector<Send> Sends;
            int32_t DuckSource = -1;
            float DuckDepthDb = -10.0f;
            float DuckThresholdDb = -40.0f;
            float DuckAttack = 0.05f;
            float DuckRelease = 0.6f;
        };
        std::vector<Bus> Buses;
    };

    class AudioEngine
    {
    public:
        static constexpr uint32_t MaxVoices = 128;
        static constexpr uint32_t BlockFrames = 256;

        static void Init(bool openDevice = true);
        static void Shutdown();
        static bool IsAvailable();

        static AudioVoiceHandle Play(const AudioPlayDesc& desc);
    private:
        static AudioVoiceHandle PlayGraph(const AudioPlayDesc& desc);
    public:
        static void Stop(AudioVoiceHandle voice, float fadeOutSeconds = 0.0f);
        static bool IsPlaying(AudioVoiceHandle voice);
        static void SetVolume(AudioVoiceHandle voice, float volume);
        static void SetPitch(AudioVoiceHandle voice, float pitch);
        static void SetPosition(AudioVoiceHandle voice, const glm::dvec3& position);
        static void SetSpatialization(AudioVoiceHandle voice, const AudioSpatialSettings& settings);
        static void SetLoop(AudioVoiceHandle voice, bool loop);
        static void SetPaused(AudioVoiceHandle voice, bool paused);
        static void SetGraphParameter(AudioVoiceHandle voice, int32_t index, float value);
        static void FireGraphParameter(AudioVoiceHandle voice, int32_t index);
        static void SetGraphConstant(AudioVoiceHandle voice, uint32_t slot, float value);
        static glm::vec2 GetVoicePeak(AudioVoiceHandle voice);
        static uint32_t GetSampleRate();

        static void SetVoiceEnvironment(AudioVoiceHandle voice, const AudioVoiceEnvironment& environment);

        static void StopGroup(AudioVoiceGroup group, float fadeOutSeconds = 0.0f);
        static void SetGroupPaused(AudioVoiceGroup group, bool paused);

        static void SetListener(const glm::dvec3& position, const glm::quat& orientation);

        static void SetHeadphones(bool headphones);
        static void SetReflections(const AudioReflectionSettings& settings);
        static float GetReflectionMilliseconds();
        struct ReflectionStats
        {
            float Milliseconds = 0.0f;
            float Peak = 0.0f, Rms = 0.0f;
            int32_t Voices = 0;
            int32_t PathingVoices = 0;
            bool ListenerReverb = false;
        };
        static ReflectionStats GetReflectionStats();
        static bool GetHeadphones();
        static void SetMaxRealVoices(uint32_t count);
        static uint32_t GetMaxRealVoices();

        static void SetMixerLayout(const AudioMixerLayoutDesc& layout);
        static void SetBusGain(uint32_t bus, float gain);
        static void SetEffectParameter(uint32_t bus, uint32_t effect, uint32_t parameter, float value);
        static void SetEffectBypass(uint32_t bus, uint32_t effect, bool bypass);
        static void SetSendGain(uint32_t bus, uint32_t send, float gain);
        static AudioBusMeter GetBusMeter(uint32_t bus);
        static float GetEffectGainReduction(uint32_t bus, uint32_t effect);
        static uint32_t GetBusCount();
        static constexpr uint32_t MaxBuses = 32;

        static void Update(float deltaSeconds);

        static AudioStats GetStats();
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
