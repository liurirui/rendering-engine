#include "AssetManager.h"

#include "Texture2D.h"
#include "Model.h"
#include "Logger.h"
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

std::shared_ptr<Texture2D> AssetManager::loadTexture2D(const std::string& path) {
    std::string resolvedPath = resolvePath(path);
    auto it = textureCache_.find(resolvedPath);
    if (it != textureCache_.end()) {
        Logger::Info("Texture cache hit. path=" + resolvedPath);
        return it->second;
    }

    Logger::Info("Loading texture. path=" + resolvedPath);
    std::shared_ptr<Texture2D> texture(renderContext_.loadTexture2D(resolvedPath.c_str()));
    textureCache_[resolvedPath] = texture;
    return texture;
}

std::shared_ptr<Texture2D> AssetManager::loadEmbeddedTexture2D(const std::string& key, const unsigned char* encodedData, int dataSize) {
    auto it = textureCache_.find(key);
    if (it != textureCache_.end()) {
        Logger::Info("Embedded texture cache hit. key=" + key);
        return it->second;
    }

    Logger::Info("Loading embedded texture. key=" + key + ", bytes=" + std::to_string(dataSize));
    std::shared_ptr<Texture2D> texture(new Texture2D(encodedData, dataSize));
    textureCache_[key] = texture;
    return texture;
}


std::shared_ptr<ModelAsset> AssetManager::loadModelAsset(const std::string& path) {
    std::string resolvedPath = resolvePath(path);
    auto it = modelCache_.find(resolvedPath);
    if (it != modelCache_.end()) {
        Logger::Info("Model cache hit. path=" + resolvedPath);
        return it->second;
    }

    Logger::Info("Loading model asset. path=" + resolvedPath);
    std::shared_ptr<ModelAsset> modelAsset = Model::loadAsset(resolvedPath, this);
    if (modelAsset) {
        modelCache_[resolvedPath] = modelAsset;
    }
    else {
        Logger::Error("Model asset load returned null. path=" + resolvedPath);
    }
    return modelAsset;
}
NAMESPACE_END