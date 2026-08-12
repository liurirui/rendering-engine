#include "MeshResource.h"

#include <Base/Logger.h>
#include <RHI/RenderContext.h>
#include <limits>

NAMESPACE_START

MeshResource::MeshResource(RenderContext& renderContext,
    const std::vector<Mesh::Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    bool hasNormals,
    bool hasTexCoords,
    bool hasTangents)
    : renderContext_(renderContext), vertexCount_(vertices.size()), indexCount_(indices.size()) {
    if (vertices.empty() || indices.empty()) {
        Logger::Error("MeshResource creation rejected empty geometry. vertices=" +
            std::to_string(vertices.size()) + ", indices=" + std::to_string(indices.size()));
        return;
    }
    if (vertices.size() * sizeof(Mesh::Vertex) > static_cast<size_t>((std::numeric_limits<int>::max)()) ||
        indices.size() * sizeof(unsigned int) > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        Logger::Error("MeshResource geometry exceeds current RHI buffer size limit.");
        return;
    }

    vbo_ = renderContext_.createVertexBuffer(vertices.data(),
        static_cast<int>(vertices.size() * sizeof(Mesh::Vertex)));
    ibo_ = renderContext_.createIndexBuffer(indices.data(),
        static_cast<int>(indices.size() * sizeof(unsigned int)));
    vao_ = renderContext_.createVertexArray(vbo_, ibo_);

    renderContext_.setUpVertexBufferLayoutInfo(vbo_, vao_, 3, sizeof(Mesh::Vertex), 0,
        offsetof(Mesh::Vertex, position) / sizeof(float));
    if (hasNormals) {
        renderContext_.setUpVertexBufferLayoutInfo(vbo_, vao_, 3, sizeof(Mesh::Vertex), 1,
            offsetof(Mesh::Vertex, normal) / sizeof(float));
    }
    if (hasTexCoords) {
        renderContext_.setUpVertexBufferLayoutInfo(vbo_, vao_, 2, sizeof(Mesh::Vertex), 2,
            offsetof(Mesh::Vertex, texCoords) / sizeof(float));
    }
    if (hasTangents) {
        renderContext_.setUpVertexBufferLayoutInfo(vbo_, vao_, 3, sizeof(Mesh::Vertex), 3,
            offsetof(Mesh::Vertex, tangent) / sizeof(float));
        renderContext_.setUpVertexBufferLayoutInfo(vbo_, vao_, 3, sizeof(Mesh::Vertex), 4,
            offsetof(Mesh::Vertex, bitangent) / sizeof(float));
    }

    if (!isValid()) {
        Logger::Error("MeshResource GPU upload produced invalid handles.");
    }
}

MeshResource::~MeshResource() {
    if (vao_) renderContext_.deleteVertexArray(vao_);
    if (vbo_) renderContext_.deleteVertexBuffer(vbo_);
    if (ibo_) renderContext_.deleteIndexBuffer(ibo_);
}

void MeshResource::draw() const {
    if (!isValid()) {
        return;
    }
    renderContext_.bindVertexArray(vao_);
    renderContext_.drawElements(static_cast<unsigned int>(indexCount_), nullptr);
    renderContext_.bindVertexArray(0);
}

NAMESPACE_END
