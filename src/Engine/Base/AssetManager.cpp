#include "AssetManager.h"

#include "Texture2D.h"
#include <RHI/RenderContext.h>
#include <algorithm>
#include <utility>

NAMESPACE_START

AssetManager::AssetManager(RenderContext& renderContext, std::string rootPath)
    : renderContext_(renderContext), rootPath_(std::move(rootPath)) {
    std::replace(rootPath_.begin(), rootPath_.end(), '\\', '/');
    while (!rootPath_.empty() && rootPath_.back() == '/') {
        rootPath_.pop_back();
    }
}

std::string AssetManager::resolvePath(const std::string& relativePath) const {
    std::string normalized = relativePath;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    if (normalized.size() > 1 && normalized[1] == ':') {
        return normalized;
    }
    while (!normalized.empty() && normalized.front() == '/') {
        normalized.erase(normalized.begin());
    }
    return rootPath_ + "/" + normalized;
}

Texture2D* AssetManager::loadTexture2D(const std::string& relativePath) {
    textures_.push_back(std::unique_ptr<Texture2D>(renderContext_.loadTexture2D(resolvePath(relativePath).c_str())));
    return textures_.back().get();
}

NAMESPACE_END
