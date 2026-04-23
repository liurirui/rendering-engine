#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include"Mesh.h"
#include"Renderable.h"
#include"Shader.h"
#include"Transform.h"
#include"Texture2D.h"
#include"Material.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using namespace std;

NAMESPACE_START

class Model {
public:
    std::vector<Mesh*>  meshes;
    std::vector<Material*> materials;
    string directory;
    bool gammaCorrection;
    GameObject* model_go;
    Transform* transform = new Transform();
    int modelNumber;
    bool isTransformDirty = false;

    Model(string const& path, bool gamma = false);
    ~Model();

private:
    
    GameObject* loadModel(const std::string& path);
    GameObject* processNode(aiNode* node, const aiScene* scene);
    Mesh* processMesh(aiMesh* mesh, const aiScene* scene);
    Material* loadMaterial(aiMaterial* mat);
};

NAMESPACE_END
