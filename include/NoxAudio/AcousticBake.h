#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "AcousticData.h"
#include "SpatialAudio.h"

namespace NoxAudio
{
    struct AcousticBakeVolume
    {
        uint64_t Key = 0;
        std::string Name;
        glm::dmat4 Box{ 1.0 };
        AcousticBakeSettings Settings;
    };

    class AcousticBaker
    {
    public:
        AcousticBaker() = default;
        ~AcousticBaker();
        AcousticBaker(const AcousticBaker&) = delete;
        AcousticBaker& operator=(const AcousticBaker&) = delete;

        void Start(const std::vector<SpatialAudio::Instance>& instances,
                   std::vector<std::pair<std::shared_ptr<const AcousticMesh>, glm::dmat4>> meshes,
                   std::vector<AcousticBakeVolume> volumes);

        // Standalone convenience overload taking raw AcousticTriangleMesh span directly without scene/mesh-instance indirection.
        void Start(std::span<const AcousticTriangleMesh> triangleMeshes, std::vector<AcousticBakeVolume> volumes);

        bool IsRunning() const { return m_Running.load(std::memory_order_acquire); }
        float GetProgress() const { return m_Progress.load(std::memory_order_relaxed); }
        std::string GetStatus() const;
        void Cancel();

        struct Result
        {
            uint64_t Key = 0;
            Ref<AcousticData> Data;
            std::string Error;
        };
        std::vector<Result> TakeResults();
        bool WasCancelled() const { return m_Cancelled.load(std::memory_order_relaxed); }

    private:
        void Run();
        void SetStatus(std::string status);

        std::vector<AcousticMeshRead> m_Reads;
        std::vector<SpatialAudio::Instance> m_Instances;
        std::vector<std::pair<std::shared_ptr<const AcousticMesh>, glm::dmat4>> m_Meshes;
        std::vector<AcousticBakeVolume> m_Volumes;
        std::vector<Result> m_Results;
        std::thread m_Thread;
        std::atomic<bool> m_Running{ false };
        std::atomic<bool> m_Cancelled{ false };
        std::atomic<float> m_Progress{ 0.0f };
        mutable std::mutex m_StatusMutex;
        std::string m_Status;
    };
}

