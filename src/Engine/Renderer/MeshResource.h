#pragma once

#include <Base/Constants.h>
#include <Base/Mesh.h>
#include <cstddef>
#include <vector>

NAMESPACE_START

class RenderContext;

// 一个 MeshAsset 对应的 GPU 几何资源。多个场景 Mesh 实例共享它；Transform 属于实例，
// MaterialAsset 属于导入材质资源。
class MeshResource final {
public:
    MeshResource(RenderContext& renderContext,
        const std::vector<Mesh::Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        bool hasNormals,
        bool hasTexCoords,
        bool hasTangents);
    ~MeshResource();

    MeshResource(const MeshResource&) = delete;
    MeshResource& operator=(const MeshResource&) = delete;

    void draw() const;
    bool isValid() const { return vao_ != 0 && vbo_ != 0 && ibo_ != 0 && indexCount_ > 0; }
    size_t vertexCount() const { return vertexCount_; }
    size_t indexCount() const { return indexCount_; }

private:
    RenderContext& renderContext_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ibo_ = 0;
    size_t vertexCount_ = 0;
    size_t indexCount_ = 0;
};

NAMESPACE_END
