#pragma once

#include "Constants.h"
#include <memory>
#include <string>
#include <unordered_map>

NAMESPACE_START

class RenderContext;
class Texture2D;

class AssetManager {
public:
    AssetManager(RenderContext& renderContext, std::string rootPath);

    const std::string& getRootPath() const { return rootPath_; }
    std::string resolvePath(const std::string& relativePath) const;

    std::shared_ptr<Texture2D> loadTexture2D(const std::string& path);
    std::shared_ptr<Texture2D> loadEmbeddedTexture2D(const std::string& key, const unsigned char* encodedData, int dataSize);

private:
    RenderContext& renderContext_;
    std::string rootPath_;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache_;
};

NAMESPACE_END