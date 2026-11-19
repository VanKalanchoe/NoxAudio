#include <NoxAudio/NoxAudio.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace NoxAudio;

namespace
{
    int g_Failures = 0;

    void Check(bool condition, const char* expr, int line)
    {
        if (!condition)
        {
            std::cerr << "[FAIL] Line " << line << ": " << expr << "\n";
            ++g_Failures;
        }
    }

#define CHECK(expr) Check((expr), #expr, __LINE__)

    std::shared_ptr<AcousticMesh> MakeBoxMesh(const glm::vec3& minP, const glm::vec3& maxP, AcousticMaterial mat)
    {
        auto mesh = std::make_shared<AcousticMesh>();
        mesh->Vertices = {
            { minP.x, minP.y, minP.z },
            { maxP.x, minP.y, minP.z },
            { maxP.x, maxP.y, minP.z },
            { minP.x, maxP.y, minP.z },
            { minP.x, minP.y, maxP.z },
            { maxP.x, minP.y, maxP.z },
            { maxP.x, maxP.y, maxP.z },
            { minP.x, maxP.y, maxP.z },
        };
        mesh->Indices = {
            // -Z
            0, 2, 1, 0, 3, 2,
            // +Z
            4, 5, 6, 4, 6, 7,
            // -X
            0, 4, 7, 0, 7, 3,
            // +X
            1, 2, 6, 1, 6, 5,
            // -Y (floor)
            0, 1, 5, 0, 5, 4,
            // +Y (ceiling)
            3, 7, 6, 3, 6, 2
        };
        mesh->TriangleMaterials.assign(mesh->Indices.size() / 3, mat);
        return mesh;
    }

    std::shared_ptr<AcousticMesh> MakeFloorQuad(float halfSize, float y, AcousticMaterial mat)
    {
        auto mesh = std::make_shared<AcousticMesh>();
        mesh->Vertices = {
            { -halfSize, y, -halfSize },
            {  halfSize, y, -halfSize },
            {  halfSize, y,  halfSize },
            { -halfSize, y,  halfSize }
        };
        mesh->Indices = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
        mesh->TriangleMaterials.assign(mesh->Indices.size() / 3, mat);
        return mesh;
    }

    void TestLogAndFileHooks()
    {
        std::cout << "[Test] Log & File Hooks...\n";
        std::vector<std::string> logs;
        SetLogCallback([&](LogLevel level, std::string_view msg)
        {
            (void)level;
            logs.emplace_back(msg);
        });
        LogMessage(LogLevel::Info, "hello-hook");
        CHECK(logs.size() == 1 && logs[0] == "hello-hook");
        SetLogCallback(nullptr);

        const std::string virtualPayload = "NoxAudioVirtualFile12345";
        SetFileReadCallback([&](const std::filesystem::path& path, std::vector<uint8_t>& outBytes) -> bool
        {
            if (path == "virtual://test.bin")
            {
                outBytes.assign(virtualPayload.begin(), virtualPayload.end());
                return true;
            }
            return false;
        });

        std::vector<uint8_t> bytes;
        CHECK(ReadFileBytes("virtual://test.bin", bytes));
        CHECK(bytes.size() == virtualPayload.size());
        auto sz = GetFileSize("virtual://test.bin");
        CHECK(sz.has_value() && *sz == virtualPayload.size());
        char slice[9] = {};
        CHECK(ReadFileRange("virtual://test.bin", 8, 7, slice));
        CHECK(std::string(slice) == "Virtual");
        CHECK(FileExists("virtual://test.bin"));
        CHECK(!FileExists("virtual://missing.bin"));
        SetFileReadCallback(nullptr);
    }

    void TestDSPAndEffects()
    {
        std::cout << "[Test] DSP & Effects...\n";
        CHECK(std::fabs(AudioDSP::DbToGain(0.0f) - 1.0f) < 1e-5f);
        CHECK(AudioDSP::DbToGain(AudioDSP::kSilenceDb) == 0.0f);
        CHECK(std::fabs(AudioDSP::DbToGain(-6.0f) - 0.501187f) < 1e-3f);

        // Attenuation curves
        CHECK(AudioAttenuationGain(AudioAttenuation::Linear, 1.0f, 11.0f, 1.0f) == 1.0f);
        CHECK(std::fabs(AudioAttenuationGain(AudioAttenuation::Linear, 1.0f, 11.0f, 6.0f) - 0.5f) < 1e-5f);
        CHECK(AudioAttenuationGain(AudioAttenuation::Linear, 1.0f, 11.0f, 11.0f) == 0.0f);

        // Procedural sine wave + Effects
        constexpr float sampleRate = 48000.0f;
        constexpr uint32_t frames = 256;
        std::vector<float> left(frames), right(frames);
        for (uint32_t i = 0; i < frames; ++i)
        {
            const float s = 0.8f * std::sin(AudioDSP::kTwoPi * 440.0f * static_cast<float>(i) / sampleRate);
            left[i] = s;
            right[i] = s;
        }

        auto eq = CreateAudioEffect(AudioEffectType::EQ, sampleRate);
        CHECK(eq != nullptr);
        eq->Process(left.data(), right.data(), frames);

        auto limiter = CreateAudioEffect(AudioEffectType::Limiter, sampleRate);
        CHECK(limiter != nullptr);
        limiter->SetParameter(1, -6.0f); // Ceiling -6 dB
        limiter->Process(left.data(), right.data(), frames);

        auto reverb = CreateAudioEffect(AudioEffectType::Reverb, sampleRate);
        CHECK(reverb != nullptr);
        reverb->Process(left.data(), right.data(), frames);

        // Convolution Reverb with procedural impulse clip
        std::vector<float> ir(512, 0.0f);
        ir[0] = 1.0f;
        ir[64] = 0.4f;
        auto irClip = AudioClip::CreateFromSamples(std::move(ir), 48000, 1, "ir.wav");
        CHECK(irClip != nullptr && irClip->GetFrameCount() == 512);
        auto conv = CreateAudioEffect(AudioEffectType::ConvolutionReverb, sampleRate, irClip.get());
        CHECK(conv != nullptr);
        conv->Process(left.data(), right.data(), frames);

        bool allFinite = true;
        float energy = 0.0f;
        for (uint32_t i = 0; i < frames; ++i)
        {
            if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                allFinite = false;
            energy += left[i] * left[i] + right[i] * right[i];
        }
        CHECK(allFinite);
        CHECK(energy > 0.0f);
    }

    void TestAudioMixer()
    {
        std::cout << "[Test] AudioMixer Serialization & Snapshots...\n";
        AudioMixerData mixer = CreateDefaultAudioMixer();
        CHECK(mixer.FindBus(kAudioBusMaster) != nullptr);
        CHECK(mixer.FindBus(kAudioBusSFX) != nullptr);
        CHECK(mixer.IsUnder(kAudioBusSFX, kAudioBusMaster));
        CHECK(mixer.WouldLoop(kAudioBusMaster, kAudioBusSFX));

        AudioMixerSnapshot snap;
        snap.Name = "Paused";
        snap.Values.push_back({ { kAudioBusSFX, AudioMixerValueKind::Volume, 0, 0 }, -12.0f });
        mixer.Snapshots.push_back(snap);

        const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "noxaudio_test_mixer.nmixer";
        CHECK(SerializeAudioMixer(tempPath, mixer));
        AudioMixerData loaded;
        CHECK(DeserializeAudioMixer(tempPath, loaded));
        CHECK(loaded.Buses.size() == mixer.Buses.size());
        CHECK(loaded.Snapshots.size() == 1 && loaded.Snapshots[0].Name == "Paused");

        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
    }

    void TestAudioGraph()
    {
        std::cout << "[Test] AudioGraph Compilation & Evaluation...\n";
        RegisterAudioGraphNodeTypes();

        NodeGraph graph = CreateEmptyAudioGraph();
        // Add an Oscillator -> Gain -> Output Left/Right
        GraphNode osc;
        osc.Id = graph.NextNodeId++;
        osc.TypeName = "Oscillator";
        osc.Properties["Frequency"] = 440.0f;
        graph.Nodes.push_back(osc);

        GraphNode gain;
        gain.Id = graph.NextNodeId++;
        gain.TypeName = "Gain";
        gain.Properties["Gain"] = 0.5f;
        graph.Nodes.push_back(gain);

        graph.Links.push_back({ osc.Id, 0, gain.Id, 0 });
        graph.Links.push_back({ gain.Id, 0, graph.OutputNode, 0 });
        graph.Links.push_back({ gain.Id, 0, graph.OutputNode, 1 });

        std::vector<AssetHandle> missing;
        std::string error;
        auto program = AudioGraphProgram::Build(graph, missing, error);
        CHECK(program != nullptr);
        CHECK(missing.empty());
        CHECK(error.empty());

        if (program)
        {
            AudioGraphInstance instance(program, 48000.0f, 42, nullptr);
            CHECK(instance.Process());
            float maxAbs = 0.0f;
            bool finite = true;
            for (uint32_t i = 0; i < kAudioGraphBlockFrames; ++i)
            {
                const float l = instance.Left()[i];
                const float r = instance.Right()[i];
                if (!std::isfinite(l) || !std::isfinite(r))
                    finite = false;
                maxAbs = std::max(maxAbs, std::fabs(l));
                CHECK(std::fabs(l - r) < 1e-6f);
            }
            CHECK(finite);
            CHECK(maxAbs > 0.1f && maxAbs <= 0.6f);
        }
    }

    void TestAudioEngineHeadless()
    {
        std::cout << "[Test] AudioEngine Headless & Mixer Integration...\n";
        AudioEngine::Init(false);
        AudioMixer::Init();
        AudioMixer::SetGameVolume(kAudioBusSFX, 0.75f);
        CHECK(std::fabs(AudioMixer::GetGameVolume(kAudioBusSFX) - 0.75f) < 1e-5f);
        AudioMixer::Update(0.016f);
        AudioEngine::Update(0.016f);
        AudioStats stats = AudioEngine::GetStats();
        CHECK(!stats.DeviceOpen);
        AudioMixer::Shutdown();
        AudioEngine::Shutdown();
    }

    void TestSpatialAudioAndAcousticBake()
    {
        std::cout << "[Test] SpatialAudio (Steam Audio HRTF, Occlusion, Reflections) & AcousticBake...\n";
        CHECK(SpatialAudio::Init(48000, 256));
        CHECK(SpatialAudio::IsAvailable());
        CHECK(SpatialAudio::Context() != nullptr);
        CHECK(SpatialAudio::Hrtf() != nullptr);

        // 1. Direct occlusion test: listener at (0, 1.5, 0), source at (0, 1.5, -10)
        SpatialAudio::SetListener({ 0.0, 1.5, 0.0 }, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        IPLSource src = SpatialAudio::CreateSource();
        CHECK(src != nullptr);

        SpatialAudio::SourceInputs inputs;
        inputs.Position = { 0.0, 1.5, -10.0 };
        inputs.Occlusion = true;
        inputs.AirAbsorption = true;
        inputs.Radius = 0.2f;

        // Open air: occlusion should be ~1.0
        SpatialAudio::UpdateScene({ 0.0, 1.5, 0.0 });
        SpatialAudio::SetSourceInputs(src, inputs);
        SpatialAudio::RunDirect();
        auto openOut = SpatialAudio::GetSourceOutputs(src);
        CHECK(openOut.Occlusion > 0.9f);

        // Place a concrete wall at z = -5 blocking line of sight
        auto wall = MakeBoxMesh({ -10.0f, 0.0f, -5.5f }, { 10.0f, 10.0f, -4.5f }, AcousticMaterial::Concrete);
        SpatialAudio::SetMeshInstance(1, wall, glm::dmat4(1.0));
        SpatialAudio::UpdateScene({ 0.0, 1.5, 0.0 });
        SpatialAudio::SetSourceInputs(src, inputs);
        SpatialAudio::RunDirect();
        auto occludedOut = SpatialAudio::GetSourceOutputs(src);
        CHECK(occludedOut.Occlusion < 0.2f);
        CHECK(SpatialAudio::GetSceneStats().Instances == 1);

        SpatialAudio::ReleaseSource(src);
        SpatialAudio::ClearInstances();

        // 2. AcousticBake with raw AcousticTriangleMesh (a floor inside a bake volume)
        auto floorMesh = MakeFloorQuad(10.0f, 0.0f, AcousticMaterial::Concrete);
        std::vector<AcousticTriangleMesh> rawMeshes;
        rawMeshes.push_back({ floorMesh, glm::dmat4(1.0) });

        AcousticBakeVolume vol;
        vol.Key = 77;
        vol.Name = "TestRoom";
        vol.Box = glm::dmat4(1.0);
        vol.Box[0][0] = 8.0;
        vol.Box[1][1] = 4.0;
        vol.Box[2][2] = 8.0;
        vol.Box[3] = glm::dvec4(0.0, 2.0, 0.0, 1.0);
        vol.Settings.Spacing = 4.0f;
        vol.Settings.Height = 1.5f;
        vol.Settings.Rays = 256;
        vol.Settings.Bounces = 2;
        vol.Settings.Duration = 0.25f;
        vol.Settings.Reverb = true;
        vol.Settings.Pathing = true;

        AcousticBaker baker;
        baker.Start(rawMeshes, { vol });
        while (baker.IsRunning())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto results = baker.TakeResults();
        CHECK(results.size() == 1);
        CHECK(results[0].Error.empty());
        CHECK(results[0].Data != nullptr);
        if (results[0].Data)
        {
            CHECK(results[0].Data->ProbeCount > 0);
            CHECK(!results[0].Data->ProbeBatch.empty());

            // Save & load round-trip
            const std::filesystem::path bakePath = std::filesystem::temp_directory_path() / "noxaudio_test_bake.nacoustics";
            CHECK(SaveAcousticData(bakePath, *results[0].Data));
            auto loaded = LoadAcousticData(bakePath);
            CHECK(loaded != nullptr);
            if (loaded)
            {
                CHECK(loaded->ProbeCount == results[0].Data->ProbeCount);
                CHECK(loaded->ProbeBatch.size() == results[0].Data->ProbeBatch.size());

                // Load probe batch into SpatialAudio
                SpatialAudio::SetProbeBatch(77, loaded);
                SpatialAudio::UpdateScene({ 0.0, 1.5, 0.0 });
                auto probeStats = SpatialAudio::GetProbeStats();
                CHECK(probeStats.Batches == 1);
                CHECK(probeStats.Active == 1);
                CHECK(probeStats.ListenerInside);
            }
            std::error_code ec;
            std::filesystem::remove(bakePath, ec);
        }

        SpatialAudio::Shutdown();
    }
}

int main()
{
    std::cout << "=== NoxAudio Standalone Verification Suite ===\n";

    TestLogAndFileHooks();
    TestDSPAndEffects();
    TestAudioMixer();
    TestAudioGraph();
    TestAudioEngineHeadless();
    TestSpatialAudioAndAcousticBake();

    if (g_Failures == 0)
    {
        std::cout << "=== ALL NOXAUDIO TESTS PASSED ===\n";
        return 0;
    }

    std::cerr << "=== NOXAUDIO TESTS FAILED (" << g_Failures << " failure(s)) ===\n";
    return 1;
}
