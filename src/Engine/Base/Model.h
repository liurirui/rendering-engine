#pragma once

#include <string>
#include <memory>
#include "AssetTypes.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_START

class AssetManager;
class GameObject;
class Texture2D;

class Model {
public:
    std::shared_ptr<ModelAsset> asset;
    GameObject* model_go = nullptr;

    Model(const std::string& path, bool gamma = false);
    Model(const std::string& path, AssetManager* assetManager, bool gamma = false);
    ~Model() = default;

    static std::shared_ptr<ModelAsset> loadAsset(const std::string& path, AssetManager* assetManager);
    static GameObject* instantiate(const std::shared_ptr<ModelAsset>& asset);

private:
    static std::shared_ptr<ModelNodeAsset> processNode(aiNode* node, const aiScene* scene, ModelAsset& modelAsset, AssetManager* assetManager, const std::string& directory);
    static std::shared_ptr<MeshAsset> processMesh(aiMesh* mesh, const aiScene* scene, ModelAsset& modelAsset, AssetManager* assetManager, const std::string& directory);
    static std::shared_ptr<MaterialAsset> loadMaterial(aiMaterial* mat, const aiScene* scene, AssetManager* assetManager, const std::string& directory);
    static std::shared_ptr<Texture2D> loadMaterialTexture(aiMaterial* mat, const aiScene* scene, AssetManager* assetManager, const std::string& directory, aiTextureType type, unsigned int index, const std::string& semanticName);
    static GameObject* instantiateNode(const std::shared_ptr<ModelNodeAsset>& node, const std::shared_ptr<ModelAsset>& asset);
};

NAMESPACE_END