#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AudioEffects.h"
#include "AudioEngine.h"
#include "AudioRef.h"
#include "AudioTypes.h"

namespace NoxAudio
{
    struct AudioMixerEffect
    {
        uint64_t Id = 0;
        AudioEffectType Type = AudioEffectType::EQ;
        bool Bypass = false;
        std::vector<float> Values;
        AssetHandle Impulse = 0;
        uint64_t Sidechain = 0;
    };

    struct AudioMixerSend
    {
        uint64_t Target = 0;
        float LevelDb = 0.0f;
        bool PreFader = false;
    };

    struct AudioMixerDuck
    {
        uint64_t Source = 0;
        float DepthDb = -10.0f;
        float ThresholdDb = -40.0f;
        float Attack = 0.05f;
        float Release = 0.6f;
    };

    struct AudioMixerBus
    {
        uint64_t Id = 0;
        std::string Name;
        uint64_t Parent = kAudioBusMaster;
        float VolumeDb = 0.0f;
        bool Mute = false;
        bool Solo = false;
        bool Bypass = false;
        bool PlaysWhilePaused = false;
        std::vector<AudioMixerEffect> Effects;
        std::vector<AudioMixerSend> Sends;
        AudioMixerDuck Duck;
    };

    enum class AudioMixerValueKind : uint8_t
    {
        Volume = 0,
        Send,
        Effect
    };

    struct AudioMixerValueKey
    {
        uint64_t Bus = 0;
        AudioMixerValueKind Kind = AudioMixerValueKind::Volume;
        uint64_t Item = 0;
        uint32_t Parameter = 0;
        bool operator==(const AudioMixerValueKey&) const = default;
    };

    struct AudioMixerSnapshot
    {
        std::string Name;
        std::vector<std::pair<AudioMixerValueKey, float>> Values;
    };

    struct AudioMixerData
    {
        std::vector<AudioMixerBus> Buses;
        std::vector<AudioMixerSnapshot> Snapshots;
        bool Headphones = true;
        uint32_t RealVoices = 64;
        AudioReflectionSettings Reflections;

        AudioMixerBus* FindBus(uint64_t id);
        const AudioMixerBus* FindBus(uint64_t id) const;
        int32_t IndexOf(uint64_t id) const;
        bool IsUnder(uint64_t bus, uint64_t ancestor) const;
        bool WouldLoop(uint64_t from, uint64_t to) const;
    };

    AudioMixerData CreateDefaultAudioMixer();
    std::vector<float> DefaultEffectValues(AudioEffectType type);
    bool SerializeAudioMixer(const std::filesystem::path& path, const AudioMixerData& data);
    bool DeserializeAudioMixer(const std::filesystem::path& path, AudioMixerData& outData);

    class AudioMixerAsset : public Asset
    {
    public:
        static AssetType GetStaticType() { return AssetType::AudioMixer; }
        AssetType GetType() const override { return GetStaticType(); }
        AudioMixerData Data;
    };

    class AudioMixer
    {
    public:
        static void Init();
        static void Shutdown();
        static void SetPath(const std::filesystem::path& path);
        static void LoadMixer(const std::filesystem::path& path, bool create = false);
        static void LoadProjectMixer(bool create = false);
        static const std::filesystem::path& GetPath();
        static bool Save();
        static bool IsDirty();

        static AudioMixerData& GetData();
        static void NotifyStructureChanged();
        static void NotifyValuesChanged();
        static void MarkDirty();

        static void SetEditedSnapshot(int32_t index);
        static int32_t GetEditedSnapshot();
        static float GetValue(const AudioMixerValueKey& key);
        static void SetValue(const AudioMixerValueKey& key, float value);
        static bool IsOverridden(const AudioMixerValueKey& key);
        static void ClearOverride(const AudioMixerValueKey& key);
        static float GetLiveValue(const AudioMixerValueKey& key);

        static void Update(float deltaSeconds);

        static const AudioMixerBus* FindBus(std::string_view name);
        static const AudioMixerBus* FindBus(uint64_t id);
        static int32_t GetBusIndex(uint64_t id);
        static void SetGameVolume(uint64_t bus, float volume);
        static float GetGameVolume(uint64_t bus);
        static void SetGameHeadphones(bool headphones);
        static bool GetHeadphones();
        static bool SetSnapshot(std::string_view name, float seconds);
        static std::string GetSnapshot();
        static void ResetGameState();

        static AudioBusMeter GetMeter(uint64_t bus);
        static float GetEffectGainReduction(uint64_t bus, uint32_t effect);
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
