#pragma once

#include "CompiledGraph.h"
#include "NodeGraph.h"

namespace NoxAudio
{
    class GraphCompiler
    {
    public:
        static CompiledGraph Compile(const NodeGraph& graph);
    };
}

