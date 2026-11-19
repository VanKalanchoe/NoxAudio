#pragma once

#include <optional>

#include "AudioRef.h"
#include "CompiledGraph.h"
#include "GraphCompiler.h"
#include "NodeGraph.h"

namespace NoxAudio
{
    class NodeGraphAsset : public Asset
    {
    public:
        AssetType GetType() const override { return Type; }

        AssetType Type = AssetType::AudioGraph;

        void Recompile()
        {
            Compiled = GraphCompiler::Compile(Graph);
            CompileVersion++;
        }

        NodeGraph Graph;
        CompiledGraph Compiled;

        uint32_t CompileVersion = 0;
        bool HasUnsavedEdits = false;
        std::optional<NodeGraph> DiskGraph;
        uint32_t DiskVersion = 0;
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
