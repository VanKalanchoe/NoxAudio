#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <phonon.h>

#include "AcousticData.h"
#include "AudioHooks.h"
#include "AudioRef.h"
#include "AudioTypes.h"

namespace NoxAudio
{
    class SpatialAudio
    {
    public:
        static bool Init(uint32_t sampleRate, uint32_t frameSize);
        static void Shutdown();
        static bool IsAvailable();

        static IPLContext Context();
        static IPLHRTF Hrtf();

        // ---- The acoustic scene (the running scene's static meshes) ----------------------------------------------------------
        using Instance = AcousticInstance;
        static void SetInstance(uint64_t key, const Instance& instance);
        static void RemoveInstance(uint64_t key);
        static void ClearInstances();
        static void SetMeshInstance(uint64_t key, std::shared_ptr<const AcousticMesh> mesh, const glm::dmat4& world);
        static void UpdateScene(const glm::dvec3& listener);
        struct SceneStats
        {
            uint32_t Instances = 0;
            uint32_t Meshes = 0;
            uint32_t Building = 0;
            uint64_t Triangles = 0;
        };
        static SceneStats GetSceneStats();
        static void MaterialsChanged();
        static uint32_t GetMaterialVersion();

        // ---- Simulation (the engine, for its 3D voices) ------------------------------------------------------------------------
        static IPLSource CreateSource();
        static void ReleaseSource(IPLSource& source);
        struct SourceInputs
        {
            glm::dvec3 Position{ 0.0 };
            bool Occlusion = true;
            bool AirAbsorption = true;
            float Radius = 0.5f;
        };
        struct SourceOutputs
        {
            float Occlusion = 1.0f;
            float Transmission[3] = { 1.0f, 1.0f, 1.0f };
            float AirAbsorption[3] = { 1.0f, 1.0f, 1.0f };
        };
        static void SetListener(const glm::dvec3& position, const glm::quat& orientation);
        static void SetSourceInputs(IPLSource source, const SourceInputs& inputs);
        static void RunDirect();
        static SourceOutputs GetSourceOutputs(IPLSource source);

        // ---- Reflections: rays bounced off the scene give each nearby voice its early reflections and reverb tail ---------------
        using ListenerReverb = AudioListenerReverb;
        struct ReflectionSettings
        {
            bool Enabled = true;
            int32_t Rays = 4096;
            int32_t Bounces = 4;
            float Duration = 1.0f;
            int32_t MaxSources = 4;
            float Interval = 0.1f;
            ListenerReverb Reverb = ListenerReverb::Baked;
            int32_t MaxPathingSources = 8;
        };
        static constexpr int32_t MaxReflectionSources = 16;
        static constexpr int32_t MaxPathingSources = 16;
        static constexpr float MaxReflectionDuration = 1.5f;
        static constexpr int32_t ReflectionOrder = 1;
        static constexpr int32_t ReflectionChannels = 4; // (order + 1)^2
        static void SetReflectionSettings(const ReflectionSettings& settings);
        static ReflectionSettings GetReflectionSettings();
        static bool ReflectionsIdle();
        static bool TakeReflectionResults();
        struct PathingRequest
        {
            int32_t Slot = -1;
            glm::dvec3 Position{ 0.0 };
            AudioAttenuation Attenuation = AudioAttenuation::Inverse;
            float MinDistance = 1.0f;
            float MaxDistance = 50.0f;
        };
        static bool StartSimulation(const std::vector<std::pair<int32_t, glm::dvec3>>& reflections, const std::vector<PathingRequest>& pathing);
        static bool GetReflectionParams(int32_t slot, IPLReflectionEffectParams& out);
        static bool GetListenerReverbParams(IPLReflectionEffectParams& out);
        static IPLSource ReflectionSource(int32_t slot);
        static IPLSource ListenerReverbSource();
        struct PathingOutputs
        {
            bool Valid = false;
            float Eq[3] = { 1.0f, 1.0f, 1.0f };
            float Sh[ReflectionChannels] = {};
        };
        static PathingOutputs GetPathingOutputs(int32_t slot);
        static float GetLastReflectionMilliseconds();

        // ---- Baked acoustics: Acoustic Probe Volumes' probe batches ------------------------------------------------------------
        static constexpr double OriginGridMeters = 1000.0;
        static glm::dvec3 GridOrigin(const glm::dvec3& world);
        static void SetProbeBatch(uint64_t key, Ref<AcousticData> data);
        static void RemoveProbeBatch(uint64_t key);
        static void ClearProbeBatches();
        struct ProbeStats
        {
            uint32_t Batches = 0;
            uint32_t Active = 0;
            uint32_t Probes = 0;
            bool ListenerInside = false;
        };
        static ProbeStats GetProbeStats();

        static IPLVector3 ToAcoustic(const glm::dvec3& world);
    };

    AcousticMaterial ResolveAcousticMaterial(AcousticMaterial chosen, float metallic, float transmission, bool blended);
}

