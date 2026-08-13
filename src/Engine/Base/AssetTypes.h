#pragma once

#include "Constants.h"
#include "Bounds.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

NAMESPACE_START

class MeshResource;

struct MeshAsset {
    // MeshAsset 只描述导入后的共享 CPU 几何、局部包围盒和材质引用；resource 是可共享的 GPU 表示。
    std::string name;
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    Bounds localBounds;
    bool hasNormals = false;
    bool hasTexCoords = false;
    bool hasTangents = false;
    std::shared_ptr<MaterialAsset> material;
    std::shared_ptr<MeshResource> resource;
};

struct ModelNodeAsset {
    // localTransform 保存 Assimp 节点矩阵，不能在实例化时丢弃，否则 FBX/glTF 层级会变形。
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
