#pragma once

#include "Constants.h"
#include "ShaderLibrary.h"
#include <memory>
#include <string>
#include <unordered_map>

NAMESPACE_START

struct ModelAsset;
struct MeshAsset;
class RenderContext;
class Texture2D;
class MeshResource;

struct MeshResourceStats {
    size_t resourceCount = 0;
    size_t uploadCount = 0;
    size_t cacheHitCount = 0;
    size_t vertexCount = 0;
    size_t indexCount = 0;
    size_t gpuBufferBytes = 0;
};

class AssetManager {
public:
    // AssetManager 是 CPU 资源和 GPU 资源的统一入口；调用者不需要知道
    // Assimp、stb_image 或 OpenGL 对象具体在哪一步创建。
    AssetManager(RenderContext& renderContext, std::string rootPath);

    const std::string& getRootPath() const { return rootPath_; }
    std::string resolvePath(const std::string& relativePath) const;

    std::shared_ptr<Texture2D> loadTexture2D(const std::string& path);
    std::shared_ptr<Texture2D> loadEmbeddedTexture2D(const std::string& key, const unsigned char* encodedData, int dataSize);
    std::shared_ptr<ModelAsset> loadModelAsset(const std::string& path);
    std::shared_ptr<MeshResource> loadMeshResource(const std::string& key, const MeshAsset& meshAsset);
    MeshResourceStats getMeshResourceStats() const;
    void logMeshResourceStats() const;
    ShaderLibrary& getShaderLibrary() { return shaderLibrary_; }
    const ShaderLibrary& getShaderLibrary() const { return shaderLibrary_; }

private:
    RenderContext& renderContext_;
    std::string rootPath_;
    ShaderLibrary shaderLibrary_;
    // 缓存 key 必须使用 resolvePath() 产生的规范路径，避免相对路径和绝对路径
    // 指向同一个文件时各自创建一份资源。
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache_;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelCache_;
    // MeshResource 的生命周期由缓存和 Mesh 实例共同持有，多个实例共享 GPU buffer。
    std::unordered_map<std::string, std::shared_ptr<MeshResource>> meshResourceCache_;
    MeshResourceStats meshResourceStats_;
};

NAMESPACE_END
