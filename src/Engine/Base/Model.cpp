#include "Model.h"
#include "AssetManager.h"
#include <RHI/RenderContext.h>
#include "Renderable.h"
#include <algorithm>

NAMESPACE_START

static std::string normalizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

Model::Model(string const& path, bool gamma)
    : Model(path, nullptr, gamma) {
}

Model::Model(string const& path, AssetManager* assetManager, bool gamma)
    : gammaCorrection(gamma), assetManager(assetManager) {
    model_go = loadModel(path);
}

Model::~Model() {
    delete transform;
}

GameObject* Model::loadModel(string const& path) {
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
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return nullptr;
    }

    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos) {
        directory = normalizePath(path.substr(0, lastSlash));
    }
    else {
        directory = "";
    }

    return processNode(scene->mRootNode, scene);
}

GameObject* Model::processNode(aiNode* node, const aiScene* scene) {
    GameObject* go = new GameObject(node->mName.C_Str());

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        go->meshes.emplace_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        GameObject* child = processNode(node->mChildren[i], scene);
        if (child) {
            go->addChildren(child);
        }
    }
    return go;
}

Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    Mesh* newMesh = new Mesh();
    newMesh->hasNormals = (mesh->mNormals != nullptr);
    newMesh->hasTexCoords = (mesh->mTextureCoords[0] != nullptr);
    newMesh->hasTangents = (mesh->mTangents != nullptr && mesh->mBitangents != nullptr);

    newMesh->name = mesh->mName.C_Str();
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Mesh::Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = newMesh->hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.texCoords = newMesh->hasTexCoords ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
        if (newMesh->hasTangents) {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        newMesh->vertices.emplace_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            newMesh->indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        newMesh->material = loadMaterial(mat, scene);
    }
    else {
        newMesh->material = new Material();
        newMesh->material->generateShader();
    }

    newMesh->setupMesh();
    return newMesh;
}

std::shared_ptr<Texture2D> Model::loadMaterialTexture(aiMaterial* mat, const aiScene* scene, aiTextureType type, unsigned int index, const std::string& semanticName) {
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
            auto it = localTextureCache.find(key);
            if (it != localTextureCache.end()) {
                return it->second;
            }
            std::shared_ptr<Texture2D> texture(new Texture2D(data, dataSize));
            localTextureCache[key] = texture;
            return texture;
        }
        cout << "WARN::ASSIMP:: unsupported uncompressed embedded texture: " << rawPath << endl;
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

    auto it = localTextureCache.find(filename);
    if (it != localTextureCache.end()) {
        return it->second;
    }
    std::shared_ptr<Texture2D> texture(new Texture2D(filename.c_str()));
    localTextureCache[filename] = texture;
    return texture;
}

Material* Model::loadMaterial(aiMaterial* mat, const aiScene* scene) {
    Material* material = new Material();

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_BASE_COLOR, 0, "baseColor")) {
        material->setDiffuseMap(texture);
    }
    else if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_DIFFUSE, 0, "diffuse")) {
        material->setDiffuseMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_NORMALS, 0, "normal")) {
        material->setNormalMap(texture);
    }
    else if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_HEIGHT, 0, "heightAsNormal")) {
        material->setNormalMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_SPECULAR, 0, "specular")) {
        material->setSpecularMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_METALNESS, 0, "metallic")) {
        material->setMetallicMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_DIFFUSE_ROUGHNESS, 0, "roughness")) {
        material->setRoughnessMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_AMBIENT_OCCLUSION, 0, "ao")) {
        material->setAoMap(texture);
    }

    if (auto texture = loadMaterialTexture(mat, scene, aiTextureType_EMISSIVE, 0, "emissive")) {
        material->setEmissiveMap(texture);
    }

    aiColor3D color(1.0f, 1.0f, 1.0f);
    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        material->diffuseColor = glm::vec3(color.r, color.g, color.b);
    }
    if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        material->specularColor = glm::vec3(color.r, color.g, color.b);
    }
    if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
        material->emissiveColor = glm::vec3(color.r, color.g, color.b);
    }

    float value = 0.0f;
    if (mat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
        material->shininess = value;
    }
    if (mat->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
        material->opacity = value;
    }

    material->generateShader();
    materials.push_back(material);
    return material;
}

NAMESPACE_END