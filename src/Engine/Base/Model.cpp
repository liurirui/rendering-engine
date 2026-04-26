#include "Model.h"
#include<stb_image.h>
#include<RHI/RenderContext.h>
#include"Texture2D.h"
#include"Renderable.h"
#include <algorithm>

NAMESPACE_START
Model::Model(string const& path, bool gamma ) : gammaCorrection(gamma)
{
    model_go = loadModel(path);
}
Model::~Model() {
    delete transform;
}

GameObject* Model::loadModel(string const& path)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        string a = importer.GetErrorString();
        return nullptr;
    }
    // retrieve the directory path of the filepath
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos) {
        directory = path.substr(0, lastSlash);
    } else {
        directory = "";
    }
    // Normalize directory separators to forward slashes (Windows accepts both)
    std::replace(directory.begin(), directory.end(), '\\', '/');

    // process ASSIMP's root node recursively
    return processNode(scene->mRootNode, scene);
}

GameObject* Model::processNode(aiNode* node, const aiScene* scene)
{
    GameObject* go = new GameObject(node->mName.C_Str());
     
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        go->meshes.emplace_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        go->addChildren(processNode(node->mChildren[i], scene));
    }
    return go;
}

Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    Mesh* new_Mesh = new Mesh();
    new_Mesh->hasNormals = (mesh->mNormals != nullptr);
    new_Mesh->hasTexCoords = (mesh->mTextureCoords[0] != nullptr);
    new_Mesh->hasTangents = (mesh->mTangents != nullptr);

    new_Mesh->name = mesh->mName.C_Str();
    for (unsigned int i = 0; i < mesh->mNumVertices; i++){
        Mesh::Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);          
        }
        else {
            vertex.texCoords = glm::vec2(0.0f);         
        }
        if (mesh->mTangents) {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }
        if (mesh->mBitangents) {
            vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z
            );
        }
        new_Mesh->vertices.emplace_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            new_Mesh->indices.push_back(face.mIndices[j]);
        }
    }
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        new_Mesh->material = loadMaterial(mat);
    }
    new_Mesh->setupMesh(); // 生成 VAO/VBO/EBO
    return new_Mesh;
}

Material* Model::loadMaterial(aiMaterial* mat) {
    Material* material = new Material();

    // Load diffuse maps
    for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
        string filename = directory.empty() ? string(str.C_Str()) : directory + "/" + string(str.C_Str());
        // Normalize path separators to forward slashes
        std::replace(filename.begin(), filename.end(), '\\', '/');
        auto texture = std::make_shared<Texture2D>(filename.c_str());
        material->setDiffuseMap(texture);
    }

    // Load specular maps
    for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); i++) {
        aiString str;
        mat->GetTexture(aiTextureType_SPECULAR, i, &str);
        string filename = directory.empty() ? string(str.C_Str()) : directory + "/" + string(str.C_Str());
        // Normalize path separators to forward slashes
        std::replace(filename.begin(), filename.end(), '\\', '/');
        auto texture = std::make_shared<Texture2D>(filename.c_str());
        material->setSpecularMap(texture);
    }

    // Load normal maps
    for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_HEIGHT); i++) {
        aiString str;
        mat->GetTexture(aiTextureType_HEIGHT, i, &str);
        string filename = directory.empty() ? string(str.C_Str()) : directory + "/" + string(str.C_Str());
        // Normalize path separators to forward slashes
        std::replace(filename.begin(), filename.end(), '\\', '/');
        auto texture = std::make_shared<Texture2D>(filename.c_str());
        material->setNormalMap(texture);
    }

    // Load other properties if needed
   /* aiColor3D color(1.f, 1.f, 1.f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    material->diffuseColor = glm::vec3(color.r, color.g, color.b);
    
   

    mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
    material->specularColor = glm::vec3(color.r, color.g, color.b);

    float shininess;
    mat->Get(AI_MATKEY_SHININESS, shininess);
    material->shininess = shininess;*/
    material->generateShader();
    materials.push_back(material);
    return material;
}

NAMESPACE_END