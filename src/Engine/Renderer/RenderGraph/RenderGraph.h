#pragma once

#ifndef RenderGraph_h
#define RenderGraph_h

#include <memory>
#include <vector>
#include <cstddef>
#include <initializer_list>
#include"RenderGraphPass.h"

namespace realtimerenderingengine {
    class RenderContext;
};

using namespace realtimerenderingengine;

class RenderGraph
{
public:

    struct Stats {
        size_t passCount = 0;
        size_t dependencyEdgeCount = 0;
        size_t reorderedPassCount = 0;
        size_t cycleCount = 0;
    };

    RenderGraph();
    virtual ~RenderGraph();

    std::vector<std::unique_ptr<RenderGraphPass>> passes;

    void execute(RenderContext* renderContext);
    void compile();
    const Stats& stats() const { return stats_; }
    void clear();

    template<typename ParameterStructType, typename ExecuteLambdaFunction>
    bool addPass(const char * passName, ParameterStructType * parameterStruct, ExecuteLambdaFunction && executeLambdaFunction);

    template<typename ParameterStructType, typename ExecuteLambdaFunction>
    bool addPass(const char* passName, ParameterStructType* parameterStruct,
        std::initializer_list<RenderGraphResourceAccess> resources,
        ExecuteLambdaFunction&& executeLambdaFunction);

private:
    std::vector<RenderGraphPass*> executionOrder_;
    Stats stats_;

};


#include "RenderGraph.inl"

#endif // !RenderGraph_h
