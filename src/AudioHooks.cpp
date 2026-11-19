#include "NoxAudio/AudioHooks.h"

#include <atomic>
#include <cstring>
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "NoxAudio/AudioClip.h"
#include "NoxAudio/NodeGraphAsset.h"

namespace NoxAudio
{
    namespace
    {
        std::mutex g_HooksMutex;
        FileReadCallback g_FileReadCallback;
        ProfilerCallbacks g_ProfilerCallbacks;
        ParallelForCallback g_ParallelForCallback;
        AcousticMeshReadCallback g_AcousticMeshReadCallback;
        std::atomic<float> g_MetersPerUnit{ 1.0f };

        std::mutex g_MeshRegistryMutex;
        std::unordered_map<uint64_t, std::shared_ptr<AcousticMesh>> g_MeshRegistry;

        std::mutex g_ClipRegistryMutex;
        std::unordered_map<AudioClipId, Ref<AudioClip>> g_ClipRegistry;
        AudioClipRegistry::ResolverCallback g_ClipResolver;

        std::mutex g_GraphRegistryMutex;
        std::unordered_map<AudioGraphId, Ref<NodeGraphAsset>> g_GraphRegistry;
        AudioGraphRegistry::ResolverCallback g_GraphResolver;
    }

    void SetFileReadCallback(FileReadCallback callback)
    {
        std::lock_guard<std::mutex> lock(g_HooksMutex);
        g_FileReadCallback = std::move(callback);
    }

    bool ReadFileBytes(const std::filesystem::path& path, std::vector<uint8_t>& outBytes)
    {
        FileReadCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_FileReadCallback;
        }

        if (callback)
            return callback(path, outBytes);

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        const std::streamsize size = file.tellg();
        if (size < 0)
            return false;
        file.seekg(0, std::ios::beg);
        outBytes.resize(static_cast<size_t>(size));
        if (size > 0 && !file.read(reinterpret_cast<char*>(outBytes.data()), size))
        {
            outBytes.clear();
            return false;
        }
        return true;
    }

    std::string ReadFileTextOrEmpty(const std::filesystem::path& path)
    {
        std::vector<uint8_t> bytes;
        if (!ReadFileBytes(path, bytes))
            return {};
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    std::optional<uint64_t> GetFileSize(const std::filesystem::path& path)
    {
        FileReadCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_FileReadCallback;
        }

        if (callback)
        {
            std::vector<uint8_t> bytes;
            if (!callback(path, bytes))
                return std::nullopt;
            return static_cast<uint64_t>(bytes.size());
        }

        std::error_code ec;
        const uint64_t size = std::filesystem::file_size(path, ec);
        if (ec)
            return std::nullopt;
        return size;
    }

    bool ReadFileRange(const std::filesystem::path& path, uint64_t offset, uint64_t length, void* outData)
    {
        if (length == 0)
            return true;
        if (!outData)
            return false;

        FileReadCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_FileReadCallback;
        }

        if (callback)
        {
            std::vector<uint8_t> bytes;
            if (!callback(path, bytes))
                return false;
            if (offset > bytes.size() || length > bytes.size() - offset)
                return false;
            std::memcpy(outData, bytes.data() + offset, static_cast<size_t>(length));
            return true;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;
        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!file)
            return false;
        file.read(static_cast<char*>(outData), static_cast<std::streamsize>(length));
        return static_cast<uint64_t>(file.gcount()) == length;
    }

    bool FileExists(const std::filesystem::path& path)
    {
        FileReadCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_FileReadCallback;
        }
        if (callback)
        {
            std::vector<uint8_t> bytes;
            return callback(path, bytes);
        }
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    void SetProfilerCallbacks(ProfilerCallbacks callbacks)
    {
        std::lock_guard<std::mutex> lock(g_HooksMutex);
        g_ProfilerCallbacks = std::move(callbacks);
    }

    const ProfilerCallbacks& GetProfilerCallbacks()
    {
        return g_ProfilerCallbacks;
    }

    void SetParallelForCallback(ParallelForCallback callback)
    {
        std::lock_guard<std::mutex> lock(g_HooksMutex);
        g_ParallelForCallback = std::move(callback);
    }

    void ParallelFor(uint32_t itemCount, const std::function<void(uint32_t index)>& body)
    {
        if (itemCount == 0 || !body)
            return;

        ParallelForCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_ParallelForCallback;
        }

        if (callback)
        {
            callback(itemCount, body);
            return;
        }

        for (uint32_t i = 0; i < itemCount; ++i)
            body(i);
    }

    void SetMetersPerUnit(float metersPerUnit)
    {
        g_MetersPerUnit.store(metersPerUnit > 0.0f ? metersPerUnit : 1.0f, std::memory_order_relaxed);
    }

    float GetMetersPerUnit()
    {
        return g_MetersPerUnit.load(std::memory_order_relaxed);
    }

    void SetAcousticMeshReadCallback(AcousticMeshReadCallback callback)
    {
        std::lock_guard<std::mutex> lock(g_HooksMutex);
        g_AcousticMeshReadCallback = std::move(callback);
    }

    AcousticMeshRead StartAcousticMeshRead(AssetHandle mesh, uint32_t firstSubmesh, uint32_t submeshCount,
                                           std::vector<AcousticMaterial> materials)
    {
        AcousticMeshReadCallback callback;
        {
            std::lock_guard<std::mutex> lock(g_HooksMutex);
            callback = g_AcousticMeshReadCallback;
        }

        if (callback)
            return callback(mesh, firstSubmesh, submeshCount, std::move(materials));

        auto resolved = AcousticMeshRegistry::Get(mesh);
        AcousticMeshRead read;
        read.IsReady = []() { return true; };
        read.Get = [resolved = std::move(resolved)]() { return resolved; };
        return read;
    }

    void AcousticMeshRegistry::Register(uint64_t meshId, std::shared_ptr<AcousticMesh> mesh)
    {
        std::lock_guard<std::mutex> lock(g_MeshRegistryMutex);
        if (mesh)
            g_MeshRegistry[meshId] = std::move(mesh);
        else
            g_MeshRegistry.erase(meshId);
    }

    void AcousticMeshRegistry::Unregister(uint64_t meshId)
    {
        std::lock_guard<std::mutex> lock(g_MeshRegistryMutex);
        g_MeshRegistry.erase(meshId);
    }

    void AcousticMeshRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(g_MeshRegistryMutex);
        g_MeshRegistry.clear();
    }

    std::shared_ptr<AcousticMesh> AcousticMeshRegistry::Get(uint64_t meshId)
    {
        std::lock_guard<std::mutex> lock(g_MeshRegistryMutex);
        auto it = g_MeshRegistry.find(meshId);
        return it != g_MeshRegistry.end() ? it->second : nullptr;
    }

    void AudioClipRegistry::Register(AudioClipId id, const Ref<AudioClip>& clip)
    {
        std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
        if (clip)
            g_ClipRegistry[id] = clip;
        else
            g_ClipRegistry.erase(id);
    }

    void AudioClipRegistry::Unregister(AudioClipId id)
    {
        std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
        g_ClipRegistry.erase(id);
    }

    void AudioClipRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
        g_ClipRegistry.clear();
    }

    Ref<AudioClip> AudioClipRegistry::Get(AudioClipId id)
    {
        ResolverCallback resolver;
        {
            std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
            if (auto it = g_ClipRegistry.find(id); it != g_ClipRegistry.end())
                return it->second;
            resolver = g_ClipResolver;
        }
        return resolver ? resolver(id) : Ref<AudioClip>{};
    }

    bool AudioClipRegistry::Contains(AudioClipId id)
    {
        std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
        return g_ClipRegistry.contains(id);
    }

    void AudioClipRegistry::SetResolver(ResolverCallback resolver)
    {
        std::lock_guard<std::mutex> lock(g_ClipRegistryMutex);
        g_ClipResolver = std::move(resolver);
    }

    void AudioGraphRegistry::Register(AudioGraphId id, const Ref<NodeGraphAsset>& graph)
    {
        std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
        if (graph)
            g_GraphRegistry[id] = graph;
        else
            g_GraphRegistry.erase(id);
    }

    void AudioGraphRegistry::Unregister(AudioGraphId id)
    {
        std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
        g_GraphRegistry.erase(id);
    }

    void AudioGraphRegistry::Clear()
    {
        std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
        g_GraphRegistry.clear();
    }

    Ref<NodeGraphAsset> AudioGraphRegistry::Get(AudioGraphId id)
    {
        ResolverCallback resolver;
        {
            std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
            if (auto it = g_GraphRegistry.find(id); it != g_GraphRegistry.end())
                return it->second;
            resolver = g_GraphResolver;
        }
        return resolver ? resolver(id) : Ref<NodeGraphAsset>{};
    }

    bool AudioGraphRegistry::Contains(AudioGraphId id)
    {
        std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
        return g_GraphRegistry.contains(id);
    }

    void AudioGraphRegistry::SetResolver(ResolverCallback resolver)
    {
        std::lock_guard<std::mutex> lock(g_GraphRegistryMutex);
        g_GraphResolver = std::move(resolver);
    }
}
