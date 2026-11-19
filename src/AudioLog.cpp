#include "NoxAudio/AudioLog.h"

#include <cstdio>
#include <mutex>

namespace NoxAudio
{
    namespace
    {
        std::mutex g_LogMutex;
        LogCallback g_LogCallback;
    }

    void SetLogCallback(LogCallback callback)
    {
        std::lock_guard<std::mutex> lock(g_LogMutex);
        g_LogCallback = std::move(callback);
    }

    void LogMessage(LogLevel level, std::string_view message)
    {
        LogCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_LogMutex);
            callback = g_LogCallback;
        }

        if (callback)
        {
            callback(level, message);
            return;
        }

        const char* prefix = "[NoxAudio]";
        switch (level)
        {
        case LogLevel::Info:  prefix = "[NoxAudio][INFO]"; break;
        case LogLevel::Warn:  prefix = "[NoxAudio][WARN]"; break;
        case LogLevel::Error: prefix = "[NoxAudio][ERROR]"; break;
        }
        std::fprintf(level == LogLevel::Error ? stderr : stdout, "%s %.*s\n",
                     prefix, static_cast<int>(message.size()), message.data());
    }
}
