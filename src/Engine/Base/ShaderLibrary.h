#pragma once

#include "Constants.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

NAMESPACE_START

class Shader;

struct ShaderHandle {
    std::string name;
    std::vector<std::string> defines;

    ShaderHandle() = default;
    explicit ShaderHandle(std::string shaderName) : name(std::move(shaderName)) {}
    ShaderHandle(std::string shaderName, std::vector<std::string> shaderDefines)
        : name(std::move(shaderName)), defines(std::move(shaderDefines)) {}
    bool isValid() const { return !name.empty(); }
};

enum class ShaderPassType {
    Forward,
    Shadow,
    Debug
};

struct ShaderPassDesc {
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;
};

struct ShaderTechniqueDesc {
    std::string name;
    std::unordered_map<int, ShaderPassDesc> passes;
};

struct ShaderLibraryStats {
    size_t techniqueCount = 0;
    size_t programCount = 0;
    size_t variantCount = 0;
    size_t successfulReloads = 0;
    size_t failedReloads = 0;
};

class ShaderLibrary {
public:
    static const char* DefaultLitShaderName();
    static const char* DepthOnlyShaderName();
    static const char* LightDebugShaderName();
    static const char* ScreenShaderName();
    static const char* PostProcessShaderName();
    static const char* IBLShaderName();

    void setRootPath(std::string rootPath);
    void registerTechnique(const ShaderTechniqueDesc& technique);
    void registerPass(const std::string& techniqueName, ShaderPassType pass, const ShaderPassDesc& desc);

    std::shared_ptr<Shader> get(const ShaderHandle& handle);
    std::shared_ptr<Shader> getOrCreate(const std::string& name);
    std::shared_ptr<Shader> getPass(const ShaderHandle& handle, ShaderPassType pass,
        const std::vector<std::string>& defines = {});

    // 保留内存源码注册接口，供 IBL 或未来运行时生成 Shader 使用。
    std::shared_ptr<Shader> registerShader(const std::string& name, const char* vertexCode,
        const char* fragmentCode, const char* geometryCode = nullptr);

    void updateHotReload(bool force = false);
    void requestReload() { reloadRequested_ = true; }
    ShaderLibraryStats getStats() const;

private:
    struct ProgramRecord {
        std::string key;
        std::string techniqueName;
        ShaderPassType pass = ShaderPassType::Forward;
        ShaderPassDesc source;
        std::vector<std::string> defines;
        std::shared_ptr<Shader> shader;
        uint64_t vertexTimestamp = 0;
        uint64_t fragmentTimestamp = 0;
        uint64_t geometryTimestamp = 0;
    };

    void registerBuiltinTechniques();
    std::shared_ptr<Shader> createProgram(const std::string& techniqueName, ShaderPassType pass,
        const std::vector<std::string>& defines);
    bool compileRecord(ProgramRecord& record, bool isReload);
    void updateRecordTimestamps(ProgramRecord& record) const;
    std::string resolvePath(const std::string& path) const;
    static std::string buildProgramKey(const std::string& techniqueName, ShaderPassType pass,
        std::vector<std::string> defines);

    std::string rootPath_;
    bool builtinsRegistered_ = false;
    std::unordered_map<std::string, ShaderTechniqueDesc> techniques_;
    std::unordered_map<std::string, ProgramRecord> programs_;
    ShaderLibraryStats stats_;
    std::chrono::steady_clock::time_point lastHotReloadCheck_{};
    bool reloadRequested_ = false;
};

NAMESPACE_END
