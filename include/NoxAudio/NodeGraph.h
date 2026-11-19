#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "NodeGraphTypes.h"

namespace NoxAudio
{
    struct NodeGraph;
    struct NodeSubGraph;

    enum class TransitionCompare : uint8_t
    {
        Equal = 0,
        NotEqual,
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        IsTrue,
        IsFalse
    };

    struct TransitionRule
    {
        std::string Parameter;
        TransitionCompare Compare = TransitionCompare::Greater;
        NodeGraphValue Value = 0.0f;
    };

    struct NodeTransition
    {
        uint32_t FromState = 0;
        uint32_t ToState = 0;
        float Duration = 0.2f;
        bool EaseInOut = true;
        bool AtStateEnd = false;
        std::vector<TransitionRule> Rules;
    };

    struct GraphNode
    {
        uint32_t Id = 0;
        std::string TypeName;
        glm::vec2 EditorPosition{0.0f};
        std::unordered_map<std::string, NodeGraphValue> Properties;
        std::vector<NodeSubGraph> SubGraphs;
        std::vector<NodeTransition> Transitions;
    };

    struct GraphLink
    {
        uint32_t FromNode = 0; uint32_t FromPin = 0;
        uint32_t ToNode = 0;   uint32_t ToPin = 0;
        std::vector<float> Route;
    };

    struct GraphParameter
    {
        std::string Name;
        NodeGraphValue DefaultValue;
        bool IsTrigger = false;
    };

    inline const char* GraphParameterTypeName(const GraphParameter& parameter)
    {
        if (parameter.IsTrigger)
            return "Trigger";
        if (std::holds_alternative<bool>(parameter.DefaultValue))
            return "Bool";
        if (std::holds_alternative<int32_t>(parameter.DefaultValue))
            return "Int";
        return "Float";
    }

    struct NodeGraph
    {
        std::string Domain;
        std::vector<GraphNode> Nodes;
        std::vector<GraphLink> Links;
        std::vector<GraphParameter> Parameters;
        uint32_t OutputNode = kInvalidGraphIndex;
        uint32_t NextNodeId = 1;
    };

    struct NodeSubGraph
    {
        uint32_t Id = 0;
        std::string Name;
        glm::vec2 EditorPosition{0.0f};
        NodeGraph Graph;
    };
}

namespace Nox
{
    using namespace NoxAudio;
}
