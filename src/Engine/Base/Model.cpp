#include "Model.h"

#include "AssetManager.h"
#include "Renderable.h"
#include "Texture2D.h"
#include "Logger.h"
#include <Renderer/MeshResource.h>
#include <assimp/pbrmaterial.h>
#include <algorithm>
#include <cctype>
#include <cmath>

NAMESPACE_START

static std::string normalizePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

static bool isGltfPath(const std::string& path) {
    const size_t extensionStart = path.find_last_of('.');
    if (extensionStart == std::string::npos) {
        return false;
    }

    std::string extension = path.substr(extensionStart);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension == ".gltf" || extension == ".glb";
}

static glm::mat4 toGlmMatrix(const aiMatrix4x4& matrix) {
    // Assimp 与 GLM 的矩阵构造约定不同，必须显式按列传入，避免节点矩阵被转置。
    return glm::mat4(
        matrix.a1, matrix.b1, matrix.c1, matrix.d1,
        matrix.a2, matrix.b2, matrix.c2, matrix.d2,
        matrix.a3, matrix.b3, matrix.c3, matrix.d3,
        matrix.a4, matrix.b4, matrix.c4, matrix.d4);
}

static bool isIdentityMatrix(const glm::mat4& matrix) {
    const glm::mat4 identity(1.0f);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (std::abs(matrix[column][row] - identity[column][row]) > 0.00001f) {
                return false;
            }
        }
    }
    return true;
}

Model::Model(const std::string& path, bool gamma)
    : Model(path, nullptr, gamma) {
}

Model::Model(const std::string& path, AssetManager* assetManager, bool gamma) {
    asset = assetManager ? assetManager->loadModelAsset(path) : loadAsset(path, nullptr);
    model_go = instantiate(asset);
}

std::shared_ptr<ModelAsset> Model::loadAsset(const std::string& path, AssetManager* assetManager) {
    // 这里先生成可缓存的 CPU ModelAsset；GPU MeshResource 由 AssetManager 随后创建。
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
        Logger::Error("Assimp model import failed. path=" + path + ", error=" + importer.GetErrorString());
        return nullptr;
    }

    std::string directory;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        directory = normalizePath(path.substr(0, lastSlash));
    }

    std::shared_ptr<ModelAsset> modelAsset(new ModelAsset());
    modelAsset->sourcePath = normalizePath(path);
    const bool sourceIsGltf = isGltfPath(path);
    modelAsset->materials.reserve(scene->mNumMaterials);
    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        modelAsset->materials.push_back(loadMaterial(scene->mMaterials[materialIndex], scene, assetManager, directory, sourceIsGltf));
    }
    modelAsset->root = processNode(scene->mRootNode, scene, *modelAsset);
    Logger::Info("Model imported. path=" + modelAsset->sourcePath +
        ", meshes=" + std::to_string(modelAsset->meshes.size()) +
        ", materials=" + std::to_string(modelAsset->materials.size()) +
        ", transformedNodes=" + std::to_string(modelAsset->transformedNodeCount));
    return modelAsset;
}

std::shared_ptr<ModelNodeAsset> Model::processNode(aiNode* node, const aiScene* scene, ModelAsset& modelAsset) {
    std::shared_ptr<ModelNodeAsset> nodeAsset(new ModelNodeAsset());
    nodeAsset->name = node->mName.C_Str();
    nodeAsset->localTransform = toGlmMatrix(node->mTransformation);
    if (!isIdentityMatrix(nodeAsset->localTransform)) {
        ++modelAsset.transformedNodeCount;
    }

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::shared_ptr<MeshAsset> meshAsset = processMesh(mesh, modelAsset);
        if (meshAsset) {
            modelAsset.meshes.push_back(meshAsset);
            nodeAsset->meshIndices.push_back(modelAsset.meshes.size() - 1);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        std::shared_ptr<ModelNodeAsset> child = processNode(node->mChildren[i], scene, modelAsset);
        if (child) {
            nodeAsset->children.push_back(child);
        }
    }
    return nodeAsset;
}

std::shared_ptr<MeshAsset> Model::processMesh(aiMesh* mesh, ModelAsset& modelAsset) {
    // 只提取导入数据，不在 Assimp 遍历阶段创建 OpenGL buffer，保持 CPU 导入逻辑可复用。
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
        meshAsset->localBounds.encapsulate(vertex.position);
        meshAsset->vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            meshAsset->indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex < modelAsset.materials.size()) {
        meshAsset->material = modelAsset.materials[mesh->mMaterialIndex];
    }
    else {
        Logger::Warn("Mesh references an invalid source material index. mesh=" + meshAsset->name +
            ", materialIndex=" + std::to_string(mesh->mMaterialIndex));
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
        Logger::Warn("Unsupported uncompressed embedded texture. path=" + rawPath);
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

std::shared_ptr<MaterialAsset> Model::loadMaterial(aiMaterial* mat, const aiScene* scene, AssetManager* assetManager, const std::string& directory, bool sourceIsGltf) {
    std::shared_ptr<MaterialAsset> materialAsset(new MaterialAsset());
    materialAsset->name = mat->GetName().C_Str();
    Material& material = materialAsset->material;
    material.name = materialAsset->name;

    // Assimp exposes glTF base-color as DIFFUSE at texture index 1.  Prefer
    // that semantic over the generic texture slots used by legacy formats.
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, "baseColor")) {
        material.setDiffuseMap(texture);
    }
    else if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_BASE_COLOR, 0, "baseColor")) {
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
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_AMBIENT, 0, "legacyReflection")) {
        material.setReflectionMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_AMBIENT_OCCLUSION, 0, "ao")) {
        material.setAoMap(texture);
    }
    if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_EMISSIVE, 0, "emissive")) {
        material.setEmissiveMap(texture);
    }

    // A material only opts into metallic-roughness when its source actually
    // declares that workflow.  OBJ/MTL, DAE and arbitrary FBX properties are
    // not reliably convertible to PBR, so they use the engine default data.
    bool hasMetallicFactor = false;
    bool hasRoughnessFactor = false;
    float value = 0.0f;
    if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, value) == AI_SUCCESS) {
        material.metallic = value;
        hasMetallicFactor = true;
    }
    if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
        material.roughness = value;
        hasRoughnessFactor = true;
    }

    const bool hasPbrTexture = mat->GetTextureCount(aiTextureType_METALNESS) > 0 ||
        mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0 ||
        mat->GetTextureCount(aiTextureType_UNKNOWN) > 0 ||
        mat->GetTextureCount(aiTextureType_DIFFUSE) > 1;
    const bool usesMetallicRoughness = sourceIsGltf || hasMetallicFactor || hasRoughnessFactor || hasPbrTexture;

    if (usesMetallicRoughness) {
        material.workflow = MaterialWorkflow::MetallicRoughness;
        // glTF defaults these factors to 1.0 when a factor is omitted.
        if (!hasMetallicFactor) {
            material.metallic = 1.0f;
        }
        if (!hasRoughnessFactor) {
            material.roughness = 1.0f;
        }
        if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_METALNESS, 0, "metallic")) {
            material.setMetallicMap(texture);
        }
        if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, aiTextureType_DIFFUSE_ROUGHNESS, 0, "roughness")) {
            material.setRoughnessMap(texture);
        }
        if (auto texture = loadMaterialTexture(mat, scene, assetManager, directory, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, "metallicRoughness")) {
            material.setMetallicRoughnessMap(texture);
        }

        aiColor4D baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, baseColorFactor) == AI_SUCCESS) {
            material.diffuseColor = glm::vec3(baseColorFactor.r, baseColorFactor.g, baseColorFactor.b);
            material.opacity = baseColorFactor.a;
        }
    }
    else {
        // Explicitly convert the old specular-gloss workflow when it is
        // actually present.  This preserves legacy PBR-like assets without
        // treating every diffuse-only OBJ/DAE material as specular-gloss.
        float shininess = 0.0f;
        const bool hasShininess = mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS;
        if (material.hasSpecularMap() || material.hasReflectionMap() || hasShininess) {
            material.workflow = MaterialWorkflow::SpecularGlossiness;
            material.metallic = 0.0f;

            if (hasShininess) {
                material.shininess = shininess;
                material.roughness = glm::clamp(glm::sqrt(2.0f / (shininess + 2.0f)), 0.04f, 1.0f);
            }

            aiColor3D color(1.0f, 1.0f, 1.0f);
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                material.diffuseColor = glm::vec3(color.r, color.g, color.b);
            }
            if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
                material.specularColor = glm::vec3(color.r, color.g, color.b);
            }
        }
    }

    const char* workflowName = material.workflow == MaterialWorkflow::MetallicRoughness ? "metallicRoughness" :
        material.workflow == MaterialWorkflow::SpecularGlossiness ? "specularGlossiness" : "engineDefault";
    Logger::Info("Material imported. name=" + material.name +
        ", workflow=" + workflowName +
        ", diffuse=" + std::to_string(material.hasDiffuseMap()) +
        ", normal=" + std::to_string(material.hasNormalMap()) +
        ", specular=" + std::to_string(material.hasSpecularMap()) +
        ", reflection=" + std::to_string(material.hasReflectionMap()) +
        ", metallic=" + std::to_string(material.metallic) +
        ", roughness=" + std::to_string(material.roughness));

    // Variant 宏属于 Shader 资源引用的一部分。当前先把法线贴图分支编译成独立 program，
    // 无法线贴图材质不再携带切线空间采样代码；后续可继续扩展 Alpha/Skinned 等关键字。
    if (material.hasNormalMap()) {
        materialAsset->shader.defines.emplace_back("MATERIAL_NORMAL_MAP");
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
    // 实例化只创建场景树和实例状态；MeshAsset 的 GPU resource 通过 shared_ptr 复用。
    GameObject* go = new GameObject(node->name);
    go->GetTransform()->SetSourceLocalMatrix(node->localTransform);

    for (size_t meshIndex : node->meshIndices) {
        if (meshIndex >= asset->meshes.size()) {
            continue;
        }
        const std::shared_ptr<MeshAsset>& meshAsset = asset->meshes[meshIndex];
        Mesh* mesh = new Mesh();
        mesh->name = meshAsset->name;
        // 实例只保存轻量 Bounds、材质与共享 GPU resource；正常路径不再复制整份 CPU 几何。
        mesh->localBounds = meshAsset->localBounds;
        mesh->hasNormals = meshAsset->hasNormals;
        mesh->hasTexCoords = meshAsset->hasTexCoords;
        mesh->hasTangents = meshAsset->hasTangents;
        mesh->materialAsset = meshAsset->material;
        mesh->resource = meshAsset->resource;
        if (mesh->resource) {
            mesh->numVertex = static_cast<unsigned int>(mesh->resource->vertexCount());
        }
        else {
            Logger::Warn("MeshAsset has no shared GPU resource; using instance upload fallback. mesh=" + meshAsset->name);
            // 无 AssetManager 的兼容路径没有共享 GPU resource，只在这种情况下复制 CPU 数据并上传。
            mesh->vertices = meshAsset->vertices;
            mesh->indices = meshAsset->indices;
            mesh->setupMesh();
        }
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
