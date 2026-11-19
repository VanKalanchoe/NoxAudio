#pragma once

#include <any>
#include <cassert>
#include <string>
#include <vector>

#include "CompiledGraph.h"

namespace NoxAudio
{
    class GraphEvalContext
    {
    public:
        void Init(const CompiledGraph& graph);

        template<typename T>
        T GetInput(uint32_t compiledNodeIndex, uint32_t inputPinIndex) const
        {
            const CompiledNode& node = m_Graph->Nodes[compiledNodeIndex];
            const CompiledPin& pin = node.Inputs[inputPinIndex];
            if (pin.Node != kInvalidGraphIndex)
            {
                const CompiledNode& source = m_Graph->Nodes[pin.Node];
                const std::any& value = m_OutputValues[source.OutputSlotBase + pin.OutputSlot];
                assert(value.has_value() && "GraphEvalContext::GetInput - source output was never written; check compiled evaluation order");
                return std::any_cast<T>(value);
            }

            if constexpr (kIsNodeGraphValueType<T>)
            {
                const std::string pinName = InputPin(*node.Type, inputPinIndex).Name;
                auto it = node.Properties.find(pinName);
                if (it == node.Properties.end())
                    return T{};
                const T* value = std::get_if<T>(&it->second);
                return value ? *value : T{};
            }
            else
            {
                return T{};
            }
        }

        template<typename T>
        void SetOutput(uint32_t compiledNodeIndex, uint32_t outputPinIndex, T value)
        {
            const CompiledNode& node = m_Graph->Nodes[compiledNodeIndex];
            m_OutputValues[node.OutputSlotBase + outputPinIndex] = std::move(value);
        }

        template<typename T>
        T& GetState(uint32_t compiledNodeIndex)
        {
            std::any& slot = m_NodeState[compiledNodeIndex];
            if (!slot.has_value())
                slot = T{};
            return *std::any_cast<T>(&slot);
        }

        template<typename T>
        T GetProperty(uint32_t compiledNodeIndex, const std::string& name, T fallback = T{}) const
        {
            static_assert(kIsNodeGraphValueType<T>, "GraphEvalContext::GetProperty - T must be a NodeGraphValue alternative");
            const CompiledNode& node = m_Graph->Nodes[compiledNodeIndex];
            auto it = node.Properties.find(name);
            if (it == node.Properties.end())
                return fallback;
            const T* value = std::get_if<T>(&it->second);
            return value ? *value : fallback;
        }

        const CompiledGraph& Graph() const { return *m_Graph; }

        void* UserData = nullptr;

    private:
        const CompiledGraph* m_Graph = nullptr;
        std::vector<std::any> m_OutputValues;
        std::vector<std::any> m_NodeState;
    };

    void EvaluateGraph(GraphEvalContext& ctx, const CompiledGraph& graph);
}

namespace Nox
{
    using namespace NoxAudio;
}
