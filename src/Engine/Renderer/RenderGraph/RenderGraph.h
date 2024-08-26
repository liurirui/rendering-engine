#pragma once

#ifndef RenderGraph_h

#include <vector>
#include"RenderGraphPass.h";

namespace realtimerenderingengine {
    class RenderContext;
};

using namespace realtimerenderingengine;

class RenderGraph
{
public:

    RenderGraph();
    virtual ~RenderGraph();

    std::vector<RenderGraphPass*> passes;

    void execute(RenderContext* renderContext);

    template<typename ParameterStructType, typename ExecuteLambdaFunction>
    bool addPass(const char * passName, ParameterStructType * parameterStruct, ExecuteLambdaFunction && executeLambdaFunction);

};


#include "RenderGraph.inl"

#endif // !RenderGraph_h
