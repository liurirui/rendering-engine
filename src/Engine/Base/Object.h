#ifndef Object_H
#define Object_H
#include<string>
#include"Constants.h"
NAMESPACE_START

class Object
{
public:
    Object();
    virtual ~Object();

    uint64_t GetInstanceID() const { return m_InstanceID; }

    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }


protected:
    std::string m_Name = "Object";

    uint64_t m_InstanceID = 0;
};

class GameObject;

class Component : public Object
{
public:
    Component(GameObject* owner) : m_GameObject(owner) {}
    Component() {}
    virtual ~Component() override = default;

    GameObject* GetGameObject() const { return m_GameObject; }

    bool IsEnabled() const { return m_Enabled; }

    void SetEnabled(bool enabled);

    virtual void Awake() {}    
    virtual void Start() {}   
    virtual void Update() {}   
    virtual void LateUpdate() {} 
    virtual void OnDestroy() {} 
    virtual void OnEnable() {}  
    virtual void OnDisable() {} 

protected:
    GameObject* m_GameObject = nullptr;
    bool m_Enabled = true;
    bool m_Started = false;
};

NAMESPACE_END

#endif //Object
