#pragma once

#include "Constants.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

NAMESPACE_START

class Shader;

struct ShaderHandle {
    std::string name;

    ShaderHandle() = default;
    explicit ShaderHandle(std::string shaderName) : name(std::move(shaderName)) {}

    bool isValid() const { return !name.empty(); }
};

class ShaderLibrary {
public:
    static const char* DefaultLitShaderName();
    static const char* DepthOnlyShaderName();
    static const char* LightDebugShaderName();

    std::shared_ptr<Shader> get(const ShaderHandle& handle);
    std::shared_ptr<Shader> getOrCreate(const std::string& name);
    std::shared_ptr<Shader> registerShader(const std::string& name, const char* vertexCode, const char* fragmentCode, const char* geometryCode = nullptr);

private:
    std::shared_ptr<Shader> createBuiltin(const std::string& name);

    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
};

NAMESPACE_END
