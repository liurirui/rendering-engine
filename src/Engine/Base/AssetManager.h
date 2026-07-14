#pragma once

#include "Constants.h"
#include <memory>
#include <string>
#include <vector>

NAMESPACE_START

class RenderContext;
class Texture2D;

class AssetManager {
public:
    AssetManager(RenderContext& renderContext, std::string rootPath);

    const std::string& getRootPath() const { return rootPath_; }
    std::string resolvePath(const std::string& relativePath) const;

    Texture2D* loadTexture2D(const std::string& relativePath);

private:
    RenderContext& renderContext_;
    std::string rootPath_;
    std::vector<std::unique_ptr<Texture2D>> textures_;
};

NAMESPACE_END
