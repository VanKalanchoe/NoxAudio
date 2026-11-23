#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include "AudioRef.h"
#include "AudioTypes.h"

namespace NoxAudio
{
    struct AcousticInstance
    {
        AssetHandle Mesh = 0;
        uint32_t FirstSubmesh = 0;
        uint32_t SubmeshCount = 1;
        std::vector<AcousticMaterial> Materials;
        glm::dmat4 World{ 1.0 };
    };

    struct AcousticBakeSettings
    {
        float Spacing = 5.0f;
        float Height = 1.5f;
        bool Reverb = true;
        bool Pathing = true;
        int32_t Rays = 16384;
        int32_t Bounces = 16;
        float Duration = 1.0f;
        int32_t VisSamples = 4;
        float VisRadius = 1.0f;
        float VisThreshold = 0.1f;
        float VisRange = 1000.0f;
        float PathRange = 1000.0f;
        bool operator==(const AcousticBakeSettings&) const = default;
    };

    class AcousticData : public Asset
    {
    public:
        static constexpr uint32_t Version = 1;

        glm::dvec3 Origin{ 0.0 };
        glm::dmat4 Volume{ 1.0 };
        uint32_t ProbeCount = 0;
        std::vector<glm::vec3> ProbePositions;
        AcousticBakeSettings Settings;
        uint64_t BakedAt = 0;
        std::vector<uint8_t> ProbeBatch;

        static AssetType GetStaticType() { return AssetType::AcousticData; }
        AssetType GetType() const override { return GetStaticType(); }

        bool Contains(const glm::dvec3& point) const;
    };

    bool SaveAcousticData(const std::filesystem::path& path, const AcousticData& data);
    Ref<AcousticData> LoadAcousticData(const std::filesystem::path& path);
}

