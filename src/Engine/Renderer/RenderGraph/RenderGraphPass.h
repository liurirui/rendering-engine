#pragma once
#include <string>

namespace realtimerenderingengine {
    class RenderContext;
};

using namespace realtimerenderingengine;

class RenderGraphPass
{
public:

    RenderGraphPass() = default;

    virtual ~RenderGraphPass() {};

    virtual void execute(RenderContext* renderContext) {};

};

template<typename ParameterStructType,typename ExecuteLambdaFunction>
class RenderGraphExecutePass : public RenderGraphPass
{
public:

    RenderGraphExecutePass(const char* name, ParameterStructType* parameterStruct, ExecuteLambdaFunction&& executeLambda)
        : _name(name), _parameterStruct(parameterStruct), _executeLambdaFunction(std::forward<ExecuteLambdaFunction>(executeLambda))
    {
        //executeLambdaFunction = std::forward<ExecuteLambdaFunction>(executeLambda);
    }

    virtual ~RenderGraphExecutePass() {};

    const std::string _name;
    void* _parameterStruct = nullptr;
    ExecuteLambdaFunction _executeLambdaFunction;

    void execute(RenderContext* renderContext) {

        this->_executeLambdaFunction(renderContext);
    }

};
