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

    // Runtime local transform.
    glm::vec3 localPosition = glm::vec3(0.0);
    glm::vec3 localRotation = glm::vec3(0.0);  // Euler angles in degrees.
    glm::vec3 localScale = glm::vec3(1.0);

    // World-space transform access.
    glm::vec3 GetPosition() const;
    void SetPosition(const glm::vec3& position);

    // Mark cached transform data dirty.
    void SetDirty() { m_IsDirty = true;  }

    // Authored local transform imported from the model hierarchy. Runtime TRS
    // is composed on top so the model instance can still be moved or scaled.
    void SetSourceLocalMatrix(const glm::mat4& matrix) { m_SourceLocalMatrix = matrix; SetDirty(); }
    const glm::mat4& GetSourceLocalMatrix() const { return m_SourceLocalMatrix; }

    // 运行时编辑矩阵放在导入矩阵左侧，既能移动实例，又不会破坏模型层级变换。
    glm::mat4 getLocalMatrix() const {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, localPosition);
        modelMatrix = glm::scale(modelMatrix, localScale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.z), glm::vec3(0, 0, 1)); // Roll
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.y), glm::vec3(0, 1, 0)); // Yaw
        modelMatrix = glm::rotate(modelMatrix, glm::radians(localRotation.x), glm::vec3(1, 0, 0)); // Pitch
        return modelMatrix * m_SourceLocalMatrix;
    }

    glm::mat4 worldMaterix = glm::mat4(1.0);

private:
    Transform* m_Parent = nullptr;
    std::vector<Transform*> m_Children;
    mutable bool m_IsDirty = true;
    mutable glm::mat4 m_LocalToWorldMatrix;
    glm::mat4 m_SourceLocalMatrix = glm::mat4(1.0f);
};
NAMESPACE_END
