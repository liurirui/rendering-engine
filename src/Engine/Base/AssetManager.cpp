#include "AssetManager.h"

#include "Texture2D.h"
#include "Model.h"
#include "Logger.h"
#include "AssetTypes.h"
#include <Renderer/MeshResource.h>
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
        for (const std::shared_ptr<MeshAsset>& meshAsset : it->second->meshes) {
            if (meshAsset && meshAsset->resource) {
                ++meshResourceStats_.cacheHitCount;
            }
        }
        Logger::Info("Model cache hit. path=" + resolvedPath);
        return it->second;
    }

    Logger::Info("Loading model asset. path=" + resolvedPath);
    std::shared_ptr<ModelAsset> modelAsset = Model::loadAsset(resolvedPath, this);
    if (modelAsset) {
        size_t uploadedResources = 0;
        for (size_t meshIndex = 0; meshIndex < modelAsset->meshes.size(); ++meshIndex) {
            const std::shared_ptr<MeshAsset>& meshAsset = modelAsset->meshes[meshIndex];
            if (!meshAsset) {
                continue;
            }
            const std::string resourceKey = resolvedPath + "#mesh:" + std::to_string(meshIndex);
            meshAsset->resource = loadMeshResource(resourceKey, *meshAsset);
            if (meshAsset->resource) {
                ++uploadedResources;
            }
        }
        modelCache_[resolvedPath] = modelAsset;
        Logger::Info("Model GPU resources ready. path=" + resolvedPath +
            ", resources=" + std::to_string(uploadedResources));
    }
    else {
        Logger::Error("Model asset load returned null. path=" + resolvedPath);
    }
    return modelAsset;
}

std::shared_ptr<MeshResource> AssetManager::loadMeshResource(const std::string& key, const MeshAsset& meshAsset) {
    auto it = meshResourceCache_.find(key);
    if (it != meshResourceCache_.end()) {
        ++meshResourceStats_.cacheHitCount;
        Logger::Info("Mesh resource cache hit. key=" + key);
        return it->second;
    }

    Logger::Info("Uploading mesh resource. key=" + key +
        ", vertices=" + std::to_string(meshAsset.vertices.size()) +
        ", indices=" + std::to_string(meshAsset.indices.size()));
    std::shared_ptr<MeshResource> resource(new MeshResource(renderContext_,
        meshAsset.vertices, meshAsset.indices, meshAsset.hasNormals,
        meshAsset.hasTexCoords, meshAsset.hasTangents));
    if (!resource->isValid()) {
        Logger::Error("Mesh resource upload failed. key=" + key);
        return nullptr;
    }
    meshResourceCache_[key] = resource;
    meshResourceStats_.resourceCount = meshResourceCache_.size();
    ++meshResourceStats_.uploadCount;
    meshResourceStats_.vertexCount += meshAsset.vertices.size();
    meshResourceStats_.indexCount += meshAsset.indices.size();
    meshResourceStats_.gpuBufferBytes += meshAsset.vertices.size() * sizeof(Mesh::Vertex) +
        meshAsset.indices.size() * sizeof(unsigned int);
    return resource;
}

MeshResourceStats AssetManager::getMeshResourceStats() const {
    MeshResourceStats stats = meshResourceStats_;
    stats.resourceCount = meshResourceCache_.size();
    return stats;
}

void AssetManager::logMeshResourceStats() const {
    const MeshResourceStats stats = getMeshResourceStats();
    Logger::Info("Mesh resource stats. resources=" + std::to_string(stats.resourceCount) +
        ", uploads=" + std::to_string(stats.uploadCount) +
        ", cacheHits=" + std::to_string(stats.cacheHitCount) +
        ", vertices=" + std::to_string(stats.vertexCount) +
        ", indices=" + std::to_string(stats.indexCount) +
        ", gpuBufferBytes=" + std::to_string(stats.gpuBufferBytes));
}
NAMESPACE_END
