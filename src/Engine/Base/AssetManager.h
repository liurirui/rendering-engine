#pragma once

#include "Constants.h"
#include "ShaderLibrary.h"
#include <memory>
#include <string>
#include <unordered_map>

NAMESPACE_START

struct ModelAsset;
class RenderContext;
class Texture2D;

class AssetManager {
public:
    AssetManager(RenderContext& renderContext, std::string rootPath);

    const std::string& getRootPath() const { return rootPath_; }
    std::string resolvePath(const std::string& relativePath) const;

    std::shared_ptr<Texture2D> loadTexture2D(const std::string& path);
    std::shared_ptr<Texture2D> loadEmbeddedTexture2D(const std::string& key, const unsigned char* encodedData, int dataSize);
    std::shared_ptr<ModelAsset> loadModelAsset(const std::string& path);
    ShaderLibrary& getShaderLibrary() { return shaderLibrary_; }
    const ShaderLibrary& getShaderLibrary() const { return shaderLibrary_; }

private:
    RenderContext& renderContext_;
    std::string rootPath_;
    ShaderLibrary shaderLibrary_;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache_;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelCache_;
};

NAMESPACE_END