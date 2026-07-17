#include "ShaderLibrary.h"

#include "Shader.h"
#include "ShaderCode.h"

NAMESPACE_START

const char* ShaderLibrary::DefaultLitShaderName() {
    return "engine/default-lit";
}

const char* ShaderLibrary::DepthOnlyShaderName() {
    return "engine/depth-only";
}

const char* ShaderLibrary::LightDebugShaderName() {
    return "engine/light-debug";
}

std::shared_ptr<Shader> ShaderLibrary::get(const ShaderHandle& handle) {
    if (!handle.isValid()) {
        return getOrCreate(DefaultLitShaderName());
    }
    return getOrCreate(handle.name);
}

std::shared_ptr<Shader> ShaderLibrary::getOrCreate(const std::string& name) {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second;
    }
    return createBuiltin(name);
}

std::shared_ptr<Shader> ShaderLibrary::registerShader(const std::string& name, const char* vertexCode, const char* fragmentCode, const char* geometryCode) {
    auto shader = std::make_shared<Shader>(vertexCode, fragmentCode, geometryCode);
    shaders_[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::createBuiltin(const std::string& name) {
    if (name == DefaultLitShaderName()) {
        return registerShader(name, Vertmodel_lighting, Fragmodel_lighting);
    }
    if (name == DepthOnlyShaderName()) {
        return registerShader(name, Vert_depth_map, Frag_depth_map);
    }
    if (name == LightDebugShaderName()) {
        return registerShader(name, Vertmodel_lighting, Fragmodel_cube);
    }
    return getOrCreate(DefaultLitShaderName());
}

NAMESPACE_END
