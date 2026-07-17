#include "Model.h"

#include "AssetManager.h"
#include "Renderable.h"
#include "Texture2D.h"
#include <iostream>
#include <algorithm>

NAMESPACE_START

static std::string normalizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

Model::Model(const std::string& path, bool gamma)
    : Model(path, nullptr, gamma) {
}

Model::Model(const std::string& path, AssetManager* assetManager, bool gamma) {
    asset = assetManager ? assetManager->loadModelAsset(path) : loadAsset(path, nullptr);
    model_go = instantiate(asset);
}

std::shared_ptr<ModelAsset> Model::loadAsset(const std::string& path, AssetManager* assetManager) {
    Assimp::Importer importer;
    const unsigned int importFlags = aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes;

    const aiScene* scene = importer.ReadFile(path, importFlags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    std::string directory;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        directory = normalizePath(path.substr(0, lastSlash));
    }

    std::shared_ptr<ModelAsset> modelAsset(new ModelAsset());
    modelAsset->sourcePath = normalizePath(path);
    modelAsset->root = processNode(scene->mRootNode, scene, *modelAsset, assetManager, directory);
    return modelAsset;
}

std::shared_ptr<ModelNodeAsset> Model::processNode(aiNode* node, const aiScene* scene, ModelAsset& modelAsset, AssetManager* assetManager, const std::string& directory) {
    std::shared_ptr<ModelNodeAsset> nodeAsset(new ModelNodeAsset());
    nodeAsset->name = node->mName.C_Str();

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::shared_ptr<MeshAsset> meshAsset = processMesh(mesh, scene, modelAsset, assetManager, directory);
        if (meshAsset) {
            modelAsset.meshes.push_back(meshAsset);
            nodeAsset->meshIndices.push_back(modelAsset.meshes.size() - 1);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        std::shared_ptr<ModelNodeAsset> child = processNode(node->mChildren[i], scene, modelAsset, assetManager, directory);
        if (child) {
            nodeAsset->children.push_back(child);
        }
    }
    return nodeAsset;
}

std::shared_ptr<MeshAsset> Model::processMesh(aiMesh* mesh, const aiScene* scene, ModelAsset& modelAsset, AssetManager* assetManager, const std::string& directory) {
    std::shared_ptr<MeshAsset> meshAsset(new MeshAsset());
    meshAsset->name = mesh->mName.C_Str();
    meshAsset->hasNormals = (mesh->mNormals != nullptr);
    meshAsset->hasTexCoords = (mesh->mTextureCoords[0] != nullptr);
    meshAsset->hasTangents = (mesh->mTangents != nullptr && mesh->mBitangents != nullptr);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Mesh::Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = meshAsset->hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.texCoords = meshAsset->hasTexCoords ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
        if (meshAsset->hasTangents) {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        meshAsset->vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            meshAsset->indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        meshAsset->material = loadMaterial(mat, scene, assetManager, directory);
        if (meshAsset->material) {
            modelAsset.materials.push_back(meshAsset->material);
        }
    }
    return meshAsset;
}

std::shared_ptr<Texture2D> Model::loadMaterialTexture(aiMaterial* mat, const aiScene* scene, AssetManager* assetManager, const std::string& directory, aiTextureType type, unsigned int index, const std::string& semanticName) {
    aiString texturePath;
    if (mat->GetTexture(type, index, &texturePath) != AI_SUCCESS) {
        return nullptr;
    }

    std::string rawPath = normalizePath(texturePath.C_Str());
    if (rawPath.empty()) {
        return nullptr;
    }

    if (const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(rawPath.c_str())) {
        if (embeddedTexture->mHeight == 0) {
            const unsigned char* data = reinterpret_cast<const unsigned char*>(embeddedTexture->pcData);
            int dataSize = static_cast<int>(embeddedTexture->mWidth);
            std::string key = "embedded:" + directory + ":" + rawPath + ":" + semanticName;
            if (assetManager) {
                return assetManager->loadEmbeddedTexture2D(key, data, dataSize);
            }
            return std::shared_ptr<Texture2D>(new Texture2D(data, dataSize));
        }
        std::cout << "WARN::ASSIMP:: unsupported uncompressed embedded texture: " << rawPath << std::endl;
        return nullptr;
    }

    std::string filename = rawPath;
    if (!(filename.size() > 1 && filename[1] == ':') && !directory.empty()) {
        filename = directory + "/" + filename;
    }
    filename = normalizePath(filename);

    if (assetManager) {
        return assetManager->loadTexture2D(filename);
    }
    return std::shared_ptr<Texture2D>(new Texture2D(filename.c_str()));
}

std::shared_ptr<MaterialAsset> Model::loadMaterial(aiMaterial* mat, const aiScene* scene, AssetManager* assetManager, const std::string& directory) {
    std::shared_ptr<MaterialAsset> materialAsset(new MaterialAsset());
    materialAsset->name = mat->GetName().C_Str();
    Material& material = materialAsset->material;
    material.name = materialAsset->name;

    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_BASE_COLOR, 0, "baseColor")) {
        material.setDiffuseMap(texture);
    }
    else if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_DIFFUSE, 0, "diffuse")) {
        material.setDiffuseMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_NORMALS, 0, "normal")) {
        material.setNormalMap(texture);
    }
    else if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_HEIGHT, 0, "heightAsNormal")) {
        material.setNormalMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_SPECULAR, 0, "specular")) {
        material.setSpecularMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_METALNESS, 0, "metallic")) {
        material.setMetallicMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_DIFFUSE_ROUGHNESS, 0, "roughness")) {
        material.setRoughnessMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_AMBIENT_OCCLUSION, 0, "ao")) {
        material.setAoMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_EMISSIVE, 0, "emissive")) {
        material.setEmissiveMap(texture);
    }

    aiColor3D color(1.0f, 1.0f, 1.0f);
    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        material.diffuseColor = glm::vec3(color.r, color.g, color.b);
    }
    if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        material.specularColor = glm::vec3(color.r, color.g, color.b);
    }
    if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
        material.emissiveColor = glm::vec3(color.r, color.g, color.b);
    }

    float value = 0.0f;
    if (mat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
        material.shininess = value;
    }
    if (mat->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
        material.opacity = value;
    }

    return materialAsset;
}

GameObject* Model::instantiate(const std::shared_ptr<ModelAsset>& asset) {
    if (!asset || !asset->root) {
        return nullptr;
    }
    return instantiateNode(asset->root, asset);
}

GameObject* Model::instantiateNode(const std::shared_ptr<ModelNodeAsset>& node, const std::shared_ptr<ModelAsset>& asset) {
    GameObject* go = new GameObject(node->name);

    for (size_t meshIndex : node->meshIndices) {
        if (meshIndex >= asset->meshes.size()) {
            continue;
        }
        const std::shared_ptr<MeshAsset>& meshAsset = asset->meshes[meshIndex];
        Mesh* mesh = new Mesh();
        mesh->name = meshAsset->name;
        mesh->vertices = meshAsset->vertices;
        mesh->indices = meshAsset->indices;
        mesh->hasNormals = meshAsset->hasNormals;
        mesh->hasTexCoords = meshAsset->hasTexCoords;
        mesh->hasTangents = meshAsset->hasTangents;
        mesh->materialAsset = meshAsset->material;
        mesh->setupMesh();
        go->meshes.push_back(mesh);
    }

    for (const auto& child : node->children) {
        GameObject* childGo = instantiateNode(child, asset);
        if (childGo) {
            go->addChildren(childGo);
        }
    }

    return go;
}

NAMESPACE_END