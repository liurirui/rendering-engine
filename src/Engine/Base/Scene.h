#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include<string>
#include<memory>
#include "Base/Light.h"
#include "Mesh.h"
#include "Base/AssetTypes.h"
#include"Base/Renderable.h"
#include"Camera.h"
class RenderGraph;
NAMESPACE_START
class AssetManager;
class Shader;
// Scene 描述“世界中有什么”，不负责选择 Shader 或绑定材质。
class Scene {
public:
	Scene(const std::string& sceneName) {
		name = sceneName;
		root = new GameObject(name);
	};
	~Scene();
	void addRenderable(Renderable* newMesh);
	void storeObjectMeshes(GameObject* go);
	void setAssetManager(AssetManager* manager) { assetManager = manager; }
	void createModel(const std::string& modelPath);
	void updateMeshTransform();

	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);

	// 场景生命周期接口。
	void Start();
	void Update();
	// 从 root 开始递归计算所有 GameObject 的 world matrix。
	void UpdateTransform(GameObject* go);
	void Render(Camera* camera, RenderGraph& rg);
	void DrawObjects(Shader& shader);

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
	const std::vector<GameObject*>& GetRenderableObjects() const { return renderableObjects; }
	DirectionLight* GetMainDirectionalLight() const;
	std::vector<PointLight*> GetPointLights() const;

	// 光源统一缓冲区管理
	void UpdateLightUBO();
	void addLight(Light* light) {
		lights.emplace_back(light);
	}
	GameObject* root = nullptr;
	Camera* mainCamera = nullptr;
	std::vector<Light*> lights;
	DirectionLight* mainDirectionalLight = nullptr;
	std::vector<GameObject*> renderableObjects;
	// 保持模型资产、材质和 MeshResource 的共享生命周期。
	std::vector<std::shared_ptr<ModelAsset>> modelAssets;
	std::string name = "scene";
private:
	AssetManager* assetManager = nullptr;
};

NAMESPACE_END
