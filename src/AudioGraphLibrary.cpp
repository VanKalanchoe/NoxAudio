#include "NoxAudio/AudioGraph.h"

#include <algorithm>
#include <unordered_map>

#include "AudioLogInternal.h"
#include "NoxAudio/AudioHooks.h"
#include "NoxAudio/NodeGraphAsset.h"

namespace NoxAudio
{
    namespace
    {
        struct CachedProgram
        {
            uint32_t Version = UINT32_MAX;
            std::vector<std::pair<AssetHandle, uint32_t>> Dependencies;
            std::shared_ptr<const AudioGraphProgram> Program;
            std::string Error;
        };
        std::unordered_map<uint64_t, CachedProgram>& Cache()
        {
            static std::unordered_map<uint64_t, CachedProgram> cache;
            return cache;
        }
    }

    std::shared_ptr<const AudioGraphProgram> GetAudioGraphProgram(AssetHandle graph, std::string* outError)
    {
        if (graph == 0)
        {
            if (outError)
                *outError = "not an audio graph";
            return nullptr;
        }
        const Ref<NodeGraphAsset> asset = AudioGraphRegistry::Get(graph);
        if (!asset || asset->GetType() != AssetType::AudioGraph)
        {
            if (outError)
                *outError = "the graph asset is not registered";
            return nullptr;
        }

        CachedProgram& cached = Cache()[graph];
        const bool dependenciesCurrent = std::all_of(cached.Dependencies.begin(), cached.Dependencies.end(), [](const auto& dependency)
        {
            const Ref<NodeGraphAsset> played = AudioGraphRegistry::Get(dependency.first);
            return played && played->CompileVersion == dependency.second;
        });
        if (cached.Version == asset->CompileVersion && dependenciesCurrent)
        {
            if (outError)
                *outError = cached.Error;
            return cached.Program;
        }

        std::vector<AssetHandle> missing;
        std::string error;
        std::vector<std::pair<AssetHandle, uint32_t>> dependencies;
        std::shared_ptr<AudioGraphProgram> program = AudioGraphProgram::Build(asset->Graph, missing, error, &dependencies);
        if (!missing.empty())
            return nullptr;
        cached.Version = asset->CompileVersion;
        cached.Dependencies = std::move(dependencies);
        cached.Program = program;
        cached.Error = program ? std::string() : error;
        if (!program)
            NOXAUDIO_WARN("Audio graph {}: cannot play it: {}", static_cast<uint64_t>(graph), error);
        if (outError)
            *outError = cached.Error;
        return cached.Program;
    }
}
