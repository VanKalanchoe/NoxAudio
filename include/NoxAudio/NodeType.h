#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "NodeGraphTypes.h"

namespace NoxAudio
{
    class GraphEvalContext;

    using NodeEvaluateFn = std::function<void(GraphEvalContext& ctx, uint32_t compiledNodeIndex)>;

    struct NodePinList
    {
        std::vector<NodePinDesc> Inputs;
        std::vector<NodePinDesc> Outputs;
        std::vector<std::pair<std::string, NodeGraphValue>> Literals;
    };

    struct NodeTypeDesc
    {
        std::string TypeName;
        std::string Domain;
        std::string Category;
        std::vector<NodePinDesc> Inputs;
        std::vector<NodePinDesc> Outputs;
        std::vector<std::pair<std::string, NodeGraphValue>> DefaultProperties;
        std::unordered_map<std::string, std::string> PropertyAssetTypeHints;
        std::unordered_map<std::string, std::pair<float, float>> PropertyRanges;
        NodeEvaluateFn Evaluate;
        bool OwnsSubGraphs = false;
        std::vector<NodePinDesc> VariableInputs;
        std::unordered_map<std::string, NodeGraphValue> VariableInputLiterals;
        uint32_t MinVariableInputs = 2;
        std::string ParameterType;
        std::unordered_map<std::string, std::vector<std::string>> PropertyEnums;
        std::function<void(const std::unordered_map<std::string, NodeGraphValue>& properties, NodePinList& out)> NodePins;
    };

    inline constexpr const char* kVariableInputCountProperty = "InputCount";

    inline uint32_t VariableInputCount(const NodeTypeDesc& type, const std::unordered_map<std::string, NodeGraphValue>& properties)
    {
        if (type.VariableInputs.empty())
            return 0;
        const auto found = properties.find(kVariableInputCountProperty);
        const int32_t* count = found != properties.end() ? std::get_if<int32_t>(&found->second) : nullptr;
        return std::max<uint32_t>(type.MinVariableInputs, count && *count > 0 ? static_cast<uint32_t>(*count) : 0u);
    }

    inline uint32_t InputPinCount(const NodeTypeDesc& type, const std::unordered_map<std::string, NodeGraphValue>& properties)
    {
        return static_cast<uint32_t>(type.Inputs.size() + type.VariableInputs.size() * VariableInputCount(type, properties));
    }

    inline NodePinDesc InputPin(const NodeTypeDesc& type, uint32_t index)
    {
        if (index < type.Inputs.size())
            return type.Inputs[index];
        const uint32_t variable = index - static_cast<uint32_t>(type.Inputs.size());
        const uint32_t groupSize = static_cast<uint32_t>(type.VariableInputs.size());
        const NodePinDesc& pin = type.VariableInputs[variable % groupSize];
        return { pin.Name + " " + std::to_string(variable / groupSize), pin.Type };
    }

    inline NodePinList ResolveNodePins(const NodeTypeDesc& type, const std::unordered_map<std::string, NodeGraphValue>& properties)
    {
        NodePinList pins;
        if (type.NodePins)
        {
            type.NodePins(properties, pins);
            return pins;
        }
        const uint32_t count = InputPinCount(type, properties);
        pins.Inputs.reserve(count);
        for (uint32_t index = 0; index < count; ++index)
            pins.Inputs.push_back(InputPin(type, index));
        pins.Outputs = type.Outputs;
        return pins;
    }

    inline constexpr const char* kRerouteNodeType = "Reroute";
    inline constexpr const char* kRerouteTypeProperty = "Type";
    void RegisterRerouteNodeType(const std::string& domain);

    class NodeTypeRegistry
    {
    public:
        static void Register(NodeTypeDesc desc);
        static const NodeTypeDesc* Find(const std::string& domain, const std::string& typeName);
        static const std::vector<NodeTypeDesc>& All();

    private:
        static std::vector<NodeTypeDesc>& Types();
    };
}

