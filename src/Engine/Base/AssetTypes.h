#pragma once

#include "Constants.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

NAMESPACE_START

struct MeshAsset {
    std::string name;
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    bool hasNormals = false;
    bool hasTexCoords = false;
    bool hasTangents = false;
    std::shared_ptr<MaterialAsset> material;
};

struct ModelNodeAsset {
    std::string name;
    glm::mat4 localTransform = glm::mat4(1.0f);
    std::vector<size_t> meshIndices;
    std::vector<std::shared_ptr<ModelNodeAsset>> children;
};

struct ModelAsset {
    std::string sourcePath;
    size_t transformedNodeCount = 0;
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<MaterialAsset>> materials;
    std::shared_ptr<ModelNodeAsset> root;
};

NAMESPACE_END
