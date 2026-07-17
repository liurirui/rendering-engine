#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Mesh.h"
#include "Renderable.h"
#include "Shader.h"
#include "Transform.h"
#include "Texture2D.h"
#include "Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using namespace std;

NAMESPACE_START

class AssetManager;

class Model {
public:
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    string directory;
    bool gammaCorrection;
    GameObject* model_go = nullptr;
    Transform* transform = new Transform();
    int modelNumber = 0;
    bool isTransformDirty = false;

    Model(string const& path, bool gamma = false);
    Model(string const& path, AssetManager* assetManager, bool gamma = false);
    ~Model();

private:
    AssetManager* assetManager = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> localTextureCache;

    GameObject* loadModel(const std::string& path);
    GameObject* processNode(aiNode* node, const aiScene* scene);
    Mesh* processMesh(aiMesh* mesh, const aiScene* scene);
    Material* loadMaterial(aiMaterial* mat, const aiScene* scene);
    std::shared_ptr<Texture2D> loadMaterialTexture(aiMaterial* mat, const aiScene* scene, aiTextureType type, unsigned int index, const std::string& semanticName);
};

NAMESPACE_END