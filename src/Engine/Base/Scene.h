#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include<string>
#include "Base/Light.h"
#include "Mesh.h"
#include"Model.h"
#include"Base/Renderable.h"
#include"Renderer/MeshRenderer.h"
#include "Base/ShaderCode.h"
#include"Camera.h"
NAMESPACE_START
class Scene {
public:
	Scene(const std::string& sceneName) {
		name = sceneName;
		root = new GameObject(name);
	};
	~Scene() = default;
	void addRenderable(Renderable* newMesh);
	void storeObjectMeshes(GameObject* go);
	void createModel(const std::string& modelPath);
	void loadFloorTexture(const std::string& TexturePath);
	void updateMeshTransform();

	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);

	//Interface function
	void Start();
	void Update();
	void UpdateTransform(GameObject* go);
	void Render(Camera* camera, RenderGraph& rg);
	void RenderObject();

	void clear(); 

	// 添加/删除物体
	void addRootChild(GameObject* go) {
		root->addChildren(go);
	}
	void removeRootChild(const std::string name) {
		root->removeChildren(name);
	};
	Light* AddLight(LightType type, const glm::vec3& param,
		const glm::vec3& color, float intensity);
	void RemoveLight(Light* light);

	const std::vector<Light*>& GetAllLights() const { return lights; }
	DirectionLight* GetMainDirectionalLight() const;
	std::vector<PointLight*> GetPointLights() const;

	// 光源统一缓冲区管理
	void UpdateLightUBO();
	void addLight(Light* light) {
		lights.emplace_back(light);
	}
	GameObject* root = nullptr;
	MeshRenderer* meshRenderer = nullptr;
	Camera* mainCamera = nullptr;
	std::vector<Light*> lights;
	DirectionLight* mainDirectionalLight = nullptr;
	std::vector<GameObject*> renderableObjects;
	std::string name = "scene";
private:
	
};

NAMESPACE_END