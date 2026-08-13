#pragma once

#include <Base/Bounds.h>
#include <Base/Constants.h>
#include <glm/glm.hpp>
#include <memory>

NAMESPACE_START

class Mesh;
class MeshResource;
struct MaterialAsset;

// RenderItem 是 Scene 提交给 Renderer 的轻量快照，不拥有 GameObject，也不暴露场景树结构。
// 当前帧绘制只需要几何、材质、世界矩阵、包围盒和少量渲染标记。
struct RenderItem {
    Mesh* mesh = nullptr;
    std::shared_ptr<MeshResource> meshResource;
    std::shared_ptr<MaterialAsset> materialAsset;
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    Bounds worldBounds;
    float cameraDistance = 0.0f;
    bool castShadows = true;
    bool receiveShadows = true;
    bool transparent = false;
};

NAMESPACE_END
