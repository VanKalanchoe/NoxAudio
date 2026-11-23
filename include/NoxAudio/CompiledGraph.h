#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "NodeGraph.h"
#include "NodeGraphTypes.h"
#include "NodeType.h"

namespace NoxAudio
{
    struct CompiledPin
    {
        uint32_t Node = kInvalidGraphIndex;
        uint32_t OutputSlot = kInvalidGraphIndex;
    };

    struct CompiledSubGraph;

    struct CompiledNode
    {
        const NodeTypeDesc* Type = nullptr;
        uint32_t SourceNodeId = 0;
        std::unordered_map<std::string, NodeGraphValue> Properties;
        std::vector<CompiledPin> Inputs;
        uint32_t OutputSlotBase = 0;
        std::vector<CompiledSubGraph> SubGraphs;
        std::vector<NodeTransition> Transitions;
    };

    struct CompiledGraph
    {
        std::vector<CompiledNode> Nodes;
        uint32_t TotalOutputSlots = 0;
        uint32_t OutputNodeIndex = kInvalidGraphIndex;
        std::vector<GraphParameter> Parameters;

        bool IsValid() const { return !Nodes.empty() && OutputNodeIndex != kInvalidGraphIndex; }
    };

    struct CompiledSubGraph
    {
        uint32_t Id = 0;
        std::string Name;
        CompiledGraph Graph;
    };
}

