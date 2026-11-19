#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "AudioRef.h"
#include "AudioTypes.h"

namespace NoxAudio
{
    class AudioClip;
    class NodeGraphAsset;
    using AudioGraphAsset = NodeGraphAsset;

    // 1. Optional file reader hook (defaults to std::ifstream; engine hooks VFS / pack files)
    using FileReadCallback = std::function<bool(const std::filesystem::path& path, std::vector<uint8_t>& outBytes)>;
    void SetFileReadCallback(FileReadCallback callback);

    bool ReadFileBytes(const std::filesystem::path& path, std::vector<uint8_t>& outBytes);
    std::string ReadFileTextOrEmpty(const std::filesystem::path& path);
    std::optional<uint64_t> GetFileSize(const std::filesystem::path& path);
    bool ReadFileRange(const std::filesystem::path& path, uint64_t offset, uint64_t length, void* outData);
    bool FileExists(const std::filesystem::path& path);

    // 2. Optional profiler hooks
    struct ProfilerCallbacks
    {
        std::function<void(const char* threadName)> OnSetThreadName;
        std::function<void(const char* zoneName)> OnZoneBegin;
        std::function<void()> OnZoneEnd;
    };
    void SetProfilerCallbacks(ProfilerCallbacks callbacks);
    const ProfilerCallbacks& GetProfilerCallbacks();

    // 3. Optional parallel dispatch hook (NoxTasks::JobSystem integration; defaults to single-threaded loop)
    using ParallelForCallback = std::function<void(uint32_t itemCount, const std::function<void(uint32_t index)>& body)>;
    void SetParallelForCallback(ParallelForCallback callback);
    void ParallelFor(uint32_t itemCount, const std::function<void(uint32_t index)>& body);

    // 4. World unit scale (defaults to 1.0f = 1 meter per world unit)
    void SetMetersPerUnit(float metersPerUnit);
    float GetMetersPerUnit();

    namespace WorldUnits
    {
        inline float ToMeters(float units) { return units * GetMetersPerUnit(); }
    }

    // 5. Raw geometry input for Steam Audio acoustic baking & spatial occlusion (no Scene/MeshImporter dependency)
    struct AcousticMesh
    {
        std::vector<glm::vec3> Vertices;
        std::vector<uint32_t>  Indices; // 3 per triangle
        std::vector<AcousticMaterial> TriangleMaterials;
    };

    struct AcousticTriangleMesh
    {
        std::shared_ptr<const AcousticMesh> Mesh;
        glm::dmat4 World{ 1.0 };
    };

    // Async mesh reader hook: in the engine, this dispatches mesh reading + meshoptimizer simplification onto NoxTasks::JobSystem::Get().Async(...)
    struct AcousticMeshRead
    {
        std::function<bool()> IsReady;
        std::function<std::shared_ptr<AcousticMesh>()> Get;
    };
    using AcousticMeshReadCallback = std::function<AcousticMeshRead(uint64_t meshId, uint32_t firstSubmesh, uint32_t submeshCount, std::vector<AcousticMaterial> materials)>;
    void SetAcousticMeshReadCallback(AcousticMeshReadCallback callback);
    AcousticMeshRead StartAcousticMeshRead(AssetHandle mesh, uint32_t firstSubmesh, uint32_t submeshCount, std::vector<AcousticMaterial> materials);

    // Standalone registry for pre-built AcousticMesh assets referenced by meshId
    class AcousticMeshRegistry
    {
    public:
        static void Register(uint64_t meshId, std::shared_ptr<AcousticMesh> mesh);
        static void Unregister(uint64_t meshId);
        static void Clear();
        static std::shared_ptr<AcousticMesh> Get(uint64_t meshId);
    };

    // 6. Standalone AudioClip & AudioGraph registries (replaces NoxCore::AssetManager)
    class AudioClipRegistry
    {
    public:
        using ResolverCallback = std::function<Ref<AudioClip>(AudioClipId id)>;

        static void Register(AudioClipId id, const Ref<AudioClip>& clip);
        static void Unregister(AudioClipId id);
        static void Clear();
        static Ref<AudioClip> Get(AudioClipId id);
        static bool Contains(AudioClipId id);
        static void SetResolver(ResolverCallback resolver);
    };

    class AudioGraphRegistry
    {
    public:
        using ResolverCallback = std::function<Ref<NodeGraphAsset>(AudioGraphId id)>;

        static void Register(AudioGraphId id, const Ref<NodeGraphAsset>& graph);
        static void Unregister(AudioGraphId id);
        static void Clear();
        static Ref<NodeGraphAsset> Get(AudioGraphId id);
        static bool Contains(AudioGraphId id);
        static void SetResolver(ResolverCallback resolver);
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
