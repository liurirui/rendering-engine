#include "RenderQueueBuilder.h"

#include "RenderQueue.h"
#include <Base/Camera.h>
#include <Base/Material.h>
#include <Base/Renderable.h>
#include <Base/Scene.h>
#include <Renderer/MeshResource.h>
#include <cmath>

NAMESPACE_START

namespace {

glm::vec4 normalizePlane(const glm::vec4& plane) {
    const float length = glm::length(glm::vec3(plane));
    return length > 0.0f ? plane / length : plane;
}

uint64_t triangleCount(const Mesh& mesh) {
    if (mesh.resource) {
        return static_cast<uint64_t>(mesh.resource->indexCount() / 3);
    }
    return static_cast<uint64_t>(mesh.indices.size() / 3);
}

} // namespace

Frustum::Frustum(const glm::mat4& viewProjection) {
    // 转置后每一列对应原矩阵的一行，可直接使用 row4 +/- rowN 提取六个裁剪平面。
    const glm::mat4 rows = glm::transpose(viewProjection);
    planes_[0] = normalizePlane(rows[3] + rows[0]); // Left
    planes_[1] = normalizePlane(rows[3] - rows[0]); // Right
    planes_[2] = normalizePlane(rows[3] + rows[1]); // Bottom
    planes_[3] = normalizePlane(rows[3] - rows[1]); // Top
    planes_[4] = normalizePlane(rows[3] + rows[2]); // Near
    planes_[5] = normalizePlane(rows[3] - rows[2]); // Far
}

bool Frustum::intersects(const Bounds& bounds) const {
    if (!bounds.isValid()) {
        return true;
    }

    for (const glm::vec4& plane : planes_) {
        // 取沿平面法线方向最远的 AABB 顶点；若它仍在平面外，整个包围盒都不可见。
        const glm::vec3 positive(
            plane.x >= 0.0f ? bounds.max.x : bounds.min.x,
            plane.y >= 0.0f ? bounds.max.y : bounds.min.y,
            plane.z >= 0.0f ? bounds.max.z : bounds.min.z);
        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f) {
            return false;
        }
    }
    return true;
}

void RenderQueueBuilder::build(const Scene& scene, const Camera& camera, const glm::mat4& projection, RenderQueue& output) const {
    output.clear();
    const Frustum frustum(projection * camera.GetViewMatrix());

    for (GameObject* gameObject : scene.GetRenderableObjects()) {
        if (!gameObject || !gameObject->IsActiveSelf() || !gameObject->GetTransform()) {
            continue;
        }

        const glm::mat4 worldMatrix = gameObject->GetTransform()->worldMaterix;
        for (Mesh* mesh : gameObject->meshes) {
            if (!mesh) {
                continue;
            }

            ++output.stats.submittedItems;
            RenderItem item;
            item.mesh = mesh;
            item.meshResource = mesh->resource;
            item.materialAsset = mesh->materialAsset;
            item.worldMatrix = worldMatrix;
            item.castShadows = mesh->castShadows;
            item.receiveShadows = mesh->receiveShadows;
            item.transparent = item.materialAsset && item.materialAsset->material.opacity < 0.999f;

            if (mesh->localBounds.isValid()) {
                item.worldBounds = mesh->localBounds.transformed(worldMatrix);
                item.cameraDistance = glm::length(item.worldBounds.center() - camera.Position);
            }
            else {
                ++output.stats.invalidBoundsItems;
                item.cameraDistance = glm::length(glm::vec3(worldMatrix[3]) - camera.Position);
            }

            // Shadow caster 不能简单按相机视锥裁掉：视野外物体仍可能把阴影投进画面。
            if (item.castShadows) {
                output.shadowCasters.push_back(item);
            }

            if (!frustum.intersects(item.worldBounds)) {
                ++output.stats.culledItems;
                continue;
            }

            ++output.stats.visibleItems;
            output.stats.visibleTriangles += triangleCount(*mesh);
            if (item.transparent) {
                output.transparentItems.push_back(item);
            }
            else {
                output.opaqueItems.push_back(item);
            }
        }
    }

    output.sort();
    output.stats.shadowItems = static_cast<uint32_t>(output.shadowCasters.size());
    output.stats.opaqueItems = static_cast<uint32_t>(output.opaqueItems.size());
    output.stats.transparentItems = static_cast<uint32_t>(output.transparentItems.size());
}

NAMESPACE_END
