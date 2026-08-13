#include "ShaderLibrary.h"

#include "Logger.h"
#include "Shader.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <windows.h>

NAMESPACE_START

namespace {

int passKey(ShaderPassType pass) {
    return static_cast<int>(pass);
}

const char* passName(ShaderPassType pass) {
    switch (pass) {
        case ShaderPassType::Forward: return "forward";
        case ShaderPassType::Shadow: return "shadow";
        case ShaderPassType::Debug: return "debug";
    }
    return "unknown";
}

bool readTextFile(const std::string& path, std::string& output) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    output = stream.str();
    return true;
}

uint64_t fileTimestamp(const std::string& path) {
    if (path.empty()) {
        return 0;
    }
    WIN32_FILE_ATTRIBUTE_DATA data {};
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
        return 0;
    }
    ULARGE_INTEGER timestamp;
    timestamp.HighPart = data.ftLastWriteTime.dwHighDateTime;
    timestamp.LowPart = data.ftLastWriteTime.dwLowDateTime;
    return timestamp.QuadPart;
}

std::string injectDefines(const std::string& source, const std::vector<std::string>& defines) {
    if (defines.empty()) {
        return source;
    }

    std::ostringstream block;
    for (const std::string& define : defines) {
        if (!define.empty()) {
            block << "#define " << define << "\n";
        }
    }

    const size_t versionEnd = source.find('\n');
    if (source.compare(0, 8, "#version") == 0 && versionEnd != std::string::npos) {
        return source.substr(0, versionEnd + 1) + block.str() + source.substr(versionEnd + 1);
    }
    return block.str() + source;
}

} // namespace

const char* ShaderLibrary::DefaultLitShaderName() { return "engine/default-lit"; }
const char* ShaderLibrary::DepthOnlyShaderName() { return "engine/depth-only"; }
const char* ShaderLibrary::LightDebugShaderName() { return "engine/light-debug"; }
const char* ShaderLibrary::ScreenShaderName() { return "engine/screen"; }
const char* ShaderLibrary::PostProcessShaderName() { return "engine/post-process"; }
const char* ShaderLibrary::IBLShaderName() { return "engine/ibl"; }

void ShaderLibrary::setRootPath(std::string rootPath) {
    std::replace(rootPath.begin(), rootPath.end(), '\\', '/');
    while (!rootPath.empty() && rootPath.back() == '/') {
        rootPath.pop_back();
    }
    rootPath_ = std::move(rootPath);
    registerBuiltinTechniques();
}

void ShaderLibrary::registerBuiltinTechniques() {
    if (builtinsRegistered_) {
        return;
    }
    builtinsRegistered_ = true;

    ShaderTechniqueDesc defaultLit;
    defaultLit.name = DefaultLitShaderName();
    defaultLit.passes[passKey(ShaderPassType::Forward)] = {
        "resources/shaders/default_lit.vert", "resources/shaders/default_lit.frag", "" };
    registerTechnique(defaultLit);

    ShaderTechniqueDesc depthOnly;
    depthOnly.name = DepthOnlyShaderName();
    depthOnly.passes[passKey(ShaderPassType::Shadow)] = {
        "resources/shaders/depth_only.vert", "resources/shaders/depth_only.frag", "" };
    registerTechnique(depthOnly);

    ShaderTechniqueDesc lightDebug;
    lightDebug.name = LightDebugShaderName();
    lightDebug.passes[passKey(ShaderPassType::Debug)] = {
        "resources/shaders/light_debug.vert", "resources/shaders/light_debug.frag", "" };
    registerTechnique(lightDebug);
}

void ShaderLibrary::registerTechnique(const ShaderTechniqueDesc& technique) {
    if (technique.name.empty() || technique.passes.empty()) {
        Logger::Error("Register technique rejected invalid description. name=" + technique.name);
        return;
    }
    techniques_[technique.name] = technique;
    stats_.techniqueCount = techniques_.size();
    Logger::Info("Register shader technique. name=" + technique.name +
        ", passes=" + std::to_string(technique.passes.size()));
}

void ShaderLibrary::registerPass(const std::string& techniqueName, ShaderPassType pass, const ShaderPassDesc& desc) {
    if (techniqueName.empty() || desc.vertexPath.empty() || desc.fragmentPath.empty()) {
        Logger::Error("Register shader pass rejected invalid description. technique=" + techniqueName);
        return;
    }
    ShaderTechniqueDesc& technique = techniques_[techniqueName];
    technique.name = techniqueName;
    technique.passes[passKey(pass)] = desc;
    stats_.techniqueCount = techniques_.size();
}

std::shared_ptr<Shader> ShaderLibrary::get(const ShaderHandle& handle) {
    return getPass(handle.isValid() ? handle : ShaderHandle(DefaultLitShaderName()), ShaderPassType::Forward,
        handle.defines);
}

std::shared_ptr<Shader> ShaderLibrary::getOrCreate(const std::string& name) {
    ShaderPassType pass = ShaderPassType::Forward;
    if (name == DepthOnlyShaderName()) pass = ShaderPassType::Shadow;
    else if (name == LightDebugShaderName()) pass = ShaderPassType::Debug;
    return getPass(ShaderHandle(name), pass);
}

std::shared_ptr<Shader> ShaderLibrary::getPass(const ShaderHandle& handle, ShaderPassType pass,
    const std::vector<std::string>& defines) {
    const std::string techniqueName = handle.isValid() ? handle.name : DefaultLitShaderName();
    const std::string key = buildProgramKey(techniqueName, pass, defines);
    auto existing = programs_.find(key);
    if (existing != programs_.end()) {
        return existing->second.shader;
    }

    std::shared_ptr<Shader> shader = createProgram(techniqueName, pass, defines);
    if (!shader && techniqueName != DefaultLitShaderName()) {
        Logger::Warn("Shader pass unavailable. technique=" + techniqueName + ", pass=" + passName(pass) +
            ", fallback=" + DefaultLitShaderName());
        return getPass(ShaderHandle(DefaultLitShaderName()), ShaderPassType::Forward, defines);
    }
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::createProgram(const std::string& techniqueName, ShaderPassType pass,
    const std::vector<std::string>& defines) {
    auto techniqueIt = techniques_.find(techniqueName);
    if (techniqueIt == techniques_.end()) {
        Logger::Error("Unknown shader technique. name=" + techniqueName);
        return nullptr;
    }
    auto passIt = techniqueIt->second.passes.find(passKey(pass));
    if (passIt == techniqueIt->second.passes.end()) {
        Logger::Error("Technique has no requested pass. name=" + techniqueName + ", pass=" + passName(pass));
        return nullptr;
    }

    ProgramRecord record;
    record.key = buildProgramKey(techniqueName, pass, defines);
    record.techniqueName = techniqueName;
    record.pass = pass;
    record.source = passIt->second;
    record.defines = defines;
    record.shader = std::make_shared<Shader>();
    record.shader->setDebugName(record.key);
    if (!compileRecord(record, false)) {
        return nullptr;
    }

    const std::string key = record.key;
    programs_[key] = std::move(record);
    stats_.programCount = programs_.size();
    if (!defines.empty()) ++stats_.variantCount;
    return programs_[key].shader;
}

bool ShaderLibrary::compileRecord(ProgramRecord& record, bool isReload) {
    const std::string vertexPath = resolvePath(record.source.vertexPath);
    const std::string fragmentPath = resolvePath(record.source.fragmentPath);
    const std::string geometryPath = resolvePath(record.source.geometryPath);
    std::string vertexSource, fragmentSource, geometrySource;
    if (!readTextFile(vertexPath, vertexSource) || !readTextFile(fragmentPath, fragmentSource) ||
        (!geometryPath.empty() && !readTextFile(geometryPath, geometrySource))) {
        Logger::Error("Shader source load failed. name=" + record.key + ", vertex=" + vertexPath +
            ", fragment=" + fragmentPath + ", geometry=" + geometryPath);
        if (isReload) {
            ++stats_.failedReloads;
            updateRecordTimestamps(record);
        }
        return false;
    }

    vertexSource = injectDefines(vertexSource, record.defines);
    fragmentSource = injectDefines(fragmentSource, record.defines);
    if (!geometrySource.empty()) geometrySource = injectDefines(geometrySource, record.defines);

    const bool success = record.shader->compileAndReplace(vertexSource.c_str(), fragmentSource.c_str(),
        geometrySource.empty() ? nullptr : geometrySource.c_str());
    if (!success) {
        if (isReload) {
            ++stats_.failedReloads;
            // 记住失败文件版本，避免每 500ms 对同一错误重复编译和刷日志。
            updateRecordTimestamps(record);
        }
        Logger::Error("Shader reload rejected; previous program remains active. name=" + record.key);
        return false;
    }

    updateRecordTimestamps(record);
    if (isReload) {
        ++stats_.successfulReloads;
        Logger::Info("Shader hot reload succeeded. name=" + record.key);
    }
    else {
        Logger::Info("Shader program ready. name=" + record.key + ", vertex=" + vertexPath +
            ", fragment=" + fragmentPath);
    }
    return true;
}

std::shared_ptr<Shader> ShaderLibrary::registerShader(const std::string& name, const char* vertexCode,
    const char* fragmentCode, const char* geometryCode) {
    auto shader = std::make_shared<Shader>(vertexCode, fragmentCode, geometryCode, name.c_str());
    if (!shader->isValid()) return nullptr;
    ProgramRecord record;
    record.key = name;
    record.techniqueName = name;
    record.shader = shader;
    programs_[name] = record;
    stats_.programCount = programs_.size();
    return shader;
}

void ShaderLibrary::updateHotReload(bool force) {
    force = force || reloadRequested_;
    reloadRequested_ = false;
    const auto now = std::chrono::steady_clock::now();
    if (!force && lastHotReloadCheck_.time_since_epoch().count() != 0 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHotReloadCheck_).count() < 500) {
        return;
    }
    lastHotReloadCheck_ = now;

    for (auto& pair : programs_) {
        ProgramRecord& record = pair.second;
        if (record.source.vertexPath.empty() || record.source.fragmentPath.empty()) continue;
        const uint64_t vertexTime = fileTimestamp(resolvePath(record.source.vertexPath));
        const uint64_t fragmentTime = fileTimestamp(resolvePath(record.source.fragmentPath));
        const uint64_t geometryTime = fileTimestamp(resolvePath(record.source.geometryPath));
        if (force || vertexTime != record.vertexTimestamp || fragmentTime != record.fragmentTimestamp ||
            geometryTime != record.geometryTimestamp) {
            compileRecord(record, true);
        }
    }
}

ShaderLibraryStats ShaderLibrary::getStats() const {
    ShaderLibraryStats result = stats_;
    result.techniqueCount = techniques_.size();
    result.programCount = programs_.size();
    return result;
}

void ShaderLibrary::updateRecordTimestamps(ProgramRecord& record) const {
    record.vertexTimestamp = fileTimestamp(resolvePath(record.source.vertexPath));
    record.fragmentTimestamp = fileTimestamp(resolvePath(record.source.fragmentPath));
    record.geometryTimestamp = fileTimestamp(resolvePath(record.source.geometryPath));
}

std::string ShaderLibrary::resolvePath(const std::string& path) const {
    if (path.empty()) return "";
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    if (normalized.size() > 1 && normalized[1] == ':') return normalized;
    return rootPath_.empty() ? normalized : rootPath_ + "/" + normalized;
}

std::string ShaderLibrary::buildProgramKey(const std::string& techniqueName, ShaderPassType pass,
    std::vector<std::string> defines) {
    std::sort(defines.begin(), defines.end());
    defines.erase(std::unique(defines.begin(), defines.end()), defines.end());
    std::string key = techniqueName + "#" + passName(pass);
    for (const std::string& define : defines) key += "+" + define;
    return key;
}

NAMESPACE_END
