#pragma once

#include <format>
#include <string_view>

#include "NoxAudio/AudioHooks.h"
#include "NoxAudio/AudioLog.h"

namespace NoxAudio::Detail
{
    struct ProfileZoneGuard
    {
        bool Active = false;
        explicit ProfileZoneGuard(const char* name)
        {
            const auto& cb = GetProfilerCallbacks();
            if (cb.OnZoneBegin)
            {
                cb.OnZoneBegin(name);
                Active = true;
            }
        }
        ~ProfileZoneGuard()
        {
            if (Active)
            {
                const auto& cb = GetProfilerCallbacks();
                if (cb.OnZoneEnd)
                    cb.OnZoneEnd();
            }
        }
        ProfileZoneGuard(const ProfileZoneGuard&) = delete;
        ProfileZoneGuard& operator=(const ProfileZoneGuard&) = delete;
    };

    inline void SetProfileThreadName(const char* name)
    {
        const auto& cb = GetProfilerCallbacks();
        if (cb.OnSetThreadName)
            cb.OnSetThreadName(name);
    }
}

#define NOXAUDIO_INFO(...)  ::NoxAudio::LogMessage(::NoxAudio::LogLevel::Info,  ::std::format(__VA_ARGS__))
#define NOXAUDIO_WARN(...)  ::NoxAudio::LogMessage(::NoxAudio::LogLevel::Warn,  ::std::format(__VA_ARGS__))
#define NOXAUDIO_ERROR(...) ::NoxAudio::LogMessage(::NoxAudio::LogLevel::Error, ::std::format(__VA_ARGS__))

#define NOXAUDIO_PROFILE_SCOPE_CONCAT2(a, b) a##b
#define NOXAUDIO_PROFILE_SCOPE_CONCAT(a, b) NOXAUDIO_PROFILE_SCOPE_CONCAT2(a, b)
#define NOXAUDIO_PROFILE_SCOPE(name) ::NoxAudio::Detail::ProfileZoneGuard NOXAUDIO_PROFILE_SCOPE_CONCAT(_noxAudioZone_, __LINE__)(name)
#define NOXAUDIO_PROFILE_THREAD(name) ::NoxAudio::Detail::SetProfileThreadName(name)
