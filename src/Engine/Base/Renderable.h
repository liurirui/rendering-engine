#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include "Mesh.h"
#include "Transform.h"
#include <glm/glm.hpp>
NAMESPACE_START
class Model;
class GameObject : public Object
{
public:
    // GameObject 拥有自己的组件、Mesh 实例和子节点；析构时递归释放整棵子树。
    GameObject(const std::string& name = "GameObject")
    {
        m_Name = name;
        // 每个 GameObject 自动创建且仅创建一个 Transform
        m_Transform = new Transform(this);
        m_Components.push_back(m_Transform);
    }

    ~GameObject() override
    {
        for (auto comp : m_Components)
        {
            delete comp;
        }
        for (auto mesh : meshes)
        {
            delete mesh;
        }
        for (auto children : child)
        {
            delete children;
        }
    }

    void setParent(GameObject* p) { parent = p; }
    void addChildren(GameObject* children) { 
        child.emplace_back(children);
        children->parent = this;
    }
    void removeChildren(std::string name) {
        for (auto it = child.begin(); it != child.end(); it++) {
            auto children = *it;
            if (children->m_Name == name) {
                children->parent = nullptr;
                child.erase(it);
            }
        }
    }

    // 快捷访问 Transform
    Transform* GetTransform() const { return m_Transform; }

    // 激活/未激活
    bool IsActiveSelf() const { return m_ActiveSelf; }
    void SetActive(bool active) { m_ActiveSelf = active; }

    // 生命周期
    void Awake();
    void Start();
    void Update();
    void LateUpdate();

    std::vector<Mesh*> meshes;
    GameObject* parent = nullptr;
    std::vector<GameObject*> child;
private:
    Transform* m_Transform = nullptr;
    std::vector<Component*> m_Components;
    bool m_ActiveSelf = true;
    bool m_ActiveInHierarchy = true;
    
};

class Renderable {
public:
    // 旧的 Renderable 包装仍用于透明排序/包围盒实验，新主流程直接使用 GameObject::meshes。
    Renderable();
    ~Renderable();

    // 初始化一个 Mesh 与透明标记的组合。
    Renderable(Mesh* mesh, bool isTranslucent);

    //Assign the model's Transform to the Renderable
    void setTransformFromModel(Model* model);

    //Calculate the bounding box center
    void calculateCenter();
    //Get the coordinates of the center of the bounding box after matrix transformation 
    glm::vec3 getWorldCenter();
    glm::vec3 boundingBoxCenter = glm::vec3(0.0f, 0.0f, 0.0f);     //The center coordinates of the bounding box
    bool needCaculateWorldCenter = true;
    glm::vec3 worldBoundingBoxCenter = glm::vec3(0.0f, 0.0f, 0.0f);

    Mesh* mesh;
    Transform* transform = new Transform();
    bool isTranslucent = false;
    int modelNumber = 0;
}; 
NAMESPACE_END
