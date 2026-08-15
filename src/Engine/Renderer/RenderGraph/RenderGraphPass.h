#pragma once
#include <string>
#include <utility>
#include <vector>

namespace realtimerenderingengine {
    class RenderContext;
};

using namespace realtimerenderingengine;

enum class RenderGraphAccess {
    Read,
    Write,
    ReadWrite
};

struct RenderGraphResourceAccess {
    std::string name;
    const void* identity = nullptr;
    RenderGraphAccess access = RenderGraphAccess::Read;

    RenderGraphResourceAccess() = default;
    RenderGraphResourceAccess(const char* resourceName, RenderGraphAccess resourceAccess,
        const void* resourceIdentity = nullptr)
        : name(resourceName ? resourceName : "unnamed"), identity(resourceIdentity), access(resourceAccess) {}
};

class RenderGraphPass
{
public:

    RenderGraphPass() = default;

    virtual ~RenderGraphPass() {};

    virtual void execute(RenderContext* renderContext) {};

    const std::string& name() const { return name_; }
    const std::vector<RenderGraphResourceAccess>& resources() const { return resources_; }
    void setDescription(std::string name, std::vector<RenderGraphResourceAccess> resources) {
        name_ = std::move(name);
        resources_ = std::move(resources);
    }

private:
    std::string name_;
    std::vector<RenderGraphResourceAccess> resources_;

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

    void describe(std::vector<RenderGraphResourceAccess> resources) {
        this->setDescription(_name, std::move(resources));
    }

    virtual ~RenderGraphExecutePass() {};

    const std::string _name;
    void* _parameterStruct = nullptr;
    ExecuteLambdaFunction _executeLambdaFunction;

    void execute(RenderContext* renderContext) {

        this->_executeLambdaFunction(renderContext);
    }

};
