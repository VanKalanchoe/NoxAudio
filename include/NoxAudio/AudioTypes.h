#pragma once

#include <cstdint>
#include <string_view>

namespace NoxAudio
{
    // The mixer's buses are the .nmixer asset's (AudioMixer.h), named by id. A new mixer starts with these, at fixed ids every
    // project shares; buses added in the Mixer panel get random ones.
    inline constexpr uint64_t kAudioBusMaster = 1;
    inline constexpr uint64_t kAudioBusMusic = 2;
    inline constexpr uint64_t kAudioBusSFX = 3;
    inline constexpr uint64_t kAudioBusVoice = 4;
    inline constexpr uint64_t kAudioBusUI = 5;
    inline constexpr uint64_t kAudioBusAmbience = 6;
    inline constexpr uint64_t kAudioBusReverb = 7;

    // How the volume falls with distance between Min Distance (full volume) and Max Distance (silent).
    // The inverse curves are brought down to exactly zero at Max Distance, so a sound never cuts off audibly there.
    enum class AudioAttenuation : uint8_t
    {
        Inverse = 0,   // Min / distance: how a point source falls off in the open (-6 dB per doubling), the default
        InverseSquare, // (Min / distance)^2: faster, for small sounds that should stay local
        Linear,        // straight from 1 at Min to 0 at Max: easy to reason about for gameplay sounds
        None           // no falloff (panned only), up to Max Distance
    };

    // The gain at `distance` (meters) between Min Distance (1) and Max Distance (0) on a curve.
    float AudioAttenuationGain(AudioAttenuation attenuation, float minDistance, float maxDistance, float distance);

    std::string_view AudioAttenuationToString(AudioAttenuation attenuation);

    // A sound asset's playback rules: how many of its instances play at once and what happens to the next one, and how it ranks
    // when more sounds play than the real-voice budget mixes.
    enum class AudioConcurrencyRule : uint8_t
    {
        PreventNew = 0,     // the new one does not start
        StopOldest,         // the one playing longest stops for it
        StopFarthest,       // the one farthest from the listener stops (the new one does not start when it would be the farthest)
        StopQuietest        // the one heard most quietly stops (the new one does not start when it would be the quietest)
    };
    std::string_view AudioConcurrencyRuleToString(AudioConcurrencyRule rule);
    AudioConcurrencyRule AudioConcurrencyRuleFromString(std::string_view name);
    struct AudioSoundSettings
    {
        float Priority = 1.0f;       // 0..100: with more sounds than the real-voice budget, the louder x higher-priority ones are mixed
        uint32_t MaxCount = 0;       // instances at once (0: no limit)
        AudioConcurrencyRule Rule = AudioConcurrencyRule::StopFarthest;
        float RetriggerTime = 0.0f;  // seconds: an instance started sooner than this after the last one is not started
        bool operator==(const AudioSoundSettings&) const = default;
    };

    // The reverb at the listener that the Acoustic Reverb effect applies (Steam Audio's listener-centric reverb).
    enum class AudioListenerReverb : uint8_t { Off = 0, Realtime, Baked };
    std::string_view AudioListenerReverbToString(AudioListenerReverb reverb);
    AudioListenerReverb AudioListenerReverbFromString(std::string_view name);
    AudioAttenuation AudioAttenuationFromString(std::string_view name);

    // How a clip is held while it plays.
    enum class AudioLoadType : uint8_t
    {
        Auto = 0,           // Decompress On Load up to 5 seconds, Streaming above
        DecompressOnLoad,   // decoded to samples when loaded: no decoding while playing, most memory (short sound effects)
        CompressedInMemory, // the file's bytes stay in memory, decoded while playing (many medium sounds, dialogue)
        Streaming           // read from disk / the pack in chunks while playing, decoded as it goes (music, ambience)
    };

    std::string_view AudioLoadTypeToString(AudioLoadType type);
    AudioLoadType AudioLoadTypeFromString(std::string_view name);

    // Who owns a voice: the playing game or the editor.
    enum class AudioVoiceGroup : uint8_t
    {
        Game = 0,
        Editor,
        Count
    };

    // How a 3D sound reaches the ears (Steam Audio): Panning for speakers, Binaural (HRTF) for headphones.
    enum class AudioSpatialization : uint8_t
    {
        Default = 0,
        Panning,
        Binaural
    };

    std::string_view AudioSpatializationToString(AudioSpatialization mode);
    AudioSpatialization AudioSpatializationFromString(std::string_view name);

    // What a surface does to sound (Steam Audio's material presets): how much it absorbs and lets through at low / mid / high
    // frequencies, how much it scatters.
    enum class AcousticMaterial : uint8_t
    {
        Auto = 0,
        Generic,
        Brick,
        Concrete,
        Ceramic,
        Gravel,
        Carpet,
        Glass,
        Plaster,
        Wood,
        Metal,
        Rock,
        Count
    };

    struct AcousticMaterialProperties
    {
        float Absorption[3];   // low (< 800 Hz), mid (800 Hz - 8 kHz), high (> 8 kHz): the energy taken by a bounce
        float Scattering;      // the share reflected diffusely
        float Transmission[3]; // the share passing through (a wall's thickness included)
    };

    std::string_view AcousticMaterialToString(AcousticMaterial material);
    AcousticMaterial AcousticMaterialFromString(std::string_view name);
    // Auto is Generic here (the caller resolves Auto from the material first).
    const AcousticMaterialProperties& GetAcousticMaterialProperties(AcousticMaterial material);

    // A playing voice: 0 is none. Slot and generation, so a handle of a voice that ended never reaches the voice now in its slot.
    using AudioVoiceHandle = uint64_t;
}

namespace Nox
{
    using namespace NoxAudio;
}
