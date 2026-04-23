#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

NAMESPACE_START
class Transform : public Component
{
public:
    Transform(GameObject* owner) : Component(owner) {}
    Transform() {}
    ~Transform() override = default;

    // 本地变换
    glm::vec3 localPosition = glm::vec3(0.0);
    glm::vec3 localRotation = glm::vec3(0.0);  // 欧拉角
    glm::vec3 localScale = glm::vec3(1.0);

    // 世界变换（自动计算）
    glm::vec3 GetPosition() const;
    void SetPosition(const glm::vec3& position);

    // 标记脏值（矩阵变化时自动更新）
    void SetDirty() { m_IsDirty = true;  }

    glm::mat4 getLocalMatrix() const {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, localPosition);
        modelMatrix = glm::scale(modelMatrix, localScale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.z), glm::vec3(0, 0, 1)); // Roll
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.y), glm::vec3(0, 1, 0)); // Yaw
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.x), glm::vec3(1, 0, 0)); // Pitch
        return modelMatrix;
    }

    glm::mat4 worldMaterix = glm::mat4(1.0);

private:
    Transform* m_Parent = nullptr;
    std::vector<Transform*> m_Children;
    mutable bool m_IsDirty = true;
    mutable glm::mat4 m_LocalToWorldMatrix;
};
NAMESPACE_END