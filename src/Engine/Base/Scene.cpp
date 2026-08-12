#include"Scene.h"
#include<iostream>
#include <algorithm>
#include "AssetManager.h"
#include "Model.h"
#include "Base/Shader.h"
#include "Logger.h"
NAMESPACE_START

// Scene 管理世界对象、灯光和资源引用；具体渲染 pass 由 Renderer 负责。
Scene::~Scene() {
	for (auto light : lights) {
		delete light;
	}
	lights.clear();
	mainDirectionalLight = nullptr;

	modelAssets.clear();

	delete root;
	root = nullptr;
	renderableObjects.clear();
}
void Scene::addRenderable(Renderable* newRenderable) {
	
}

float Scene::calculateDistance(glm::vec3 cameraPosition,glm::vec3 meshPosition) {
	return glm::length(cameraPosition - meshPosition);
}

// sort translucent meshes by their distance to the camera (far to near)
void Scene::sortTranslucentMeshes(glm::vec3 cameraPosition) {
	
}

void Scene::storeObjectMeshes(GameObject* go) {
	// 预先收集包含 Mesh 的节点，Renderer 每帧无需重复遍历完整场景树。
	if (!go->meshes.empty()) renderableObjects.emplace_back(go);
	for (auto children : go->child) {
		storeObjectMeshes(children);
	}
}

// 通过 AssetManager 获取可共享资源，再实例化为场景 GameObject 树。
void Scene::createModel(const std::string& modelPath){
	std::shared_ptr<ModelAsset> modelAsset = assetManager
		? assetManager->loadModelAsset(modelPath)
		: Model::loadAsset(modelPath, nullptr);
	if (!modelAsset) {
		Logger::Error("Scene createModel failed: model asset is null. path=" + modelPath);
		return;
	}

	GameObject* modelGo = Model::instantiate(modelAsset);
	if (!modelGo) {
		Logger::Error("Scene createModel failed: instantiate returned null. path=" + modelPath);
		return;
	}

	modelAssets.emplace_back(modelAsset);
	addRootChild(modelGo);
	storeObjectMeshes(modelGo);
	Logger::Info("Scene model created. path=" + modelPath);
}


void Scene::updateMeshTransform() {
	
}

DirectionLight* Scene::GetMainDirectionalLight()const {
	if (mainDirectionalLight != nullptr) return mainDirectionalLight;
	else return nullptr;
}

Light* Scene::AddLight(LightType type, const glm::vec3& param, const glm::vec3& color, float intensity) {
	Light* newLight = nullptr;

	switch (type) {
		case LightType::Direction: {
			DirectionLight* dirLight = new DirectionLight(param, color, intensity);
			newLight = dirLight;

			if (!mainDirectionalLight) {
				mainDirectionalLight = static_cast<DirectionLight*>(newLight);
			}
			break;
		}
		case LightType::Point: {
			PointLight* pointLight = new PointLight(param, color, intensity);
			newLight = pointLight;
			break;
		}
		case LightType::Spot: {
			// 待添加
			break;
		}
	}
	if (newLight) {
		lights.push_back(newLight);
	}
	return newLight;
}

void Scene::Start() {
	
}

void Scene::Update() {
	UpdateTransform(root);
	//updateMeshTransform();
}

// 父节点矩阵递归向下传播，Transform 内部再组合运行时 TRS 与导入矩阵。
void Scene::UpdateTransform(GameObject* go) {
	if (!go->GetTransform()) return;
	glm::mat4 parentWorldMaterix = glm::mat4(1.0);
	if (go->parent) parentWorldMaterix = go->parent->GetTransform()->worldMaterix;
	go->GetTransform()->worldMaterix = parentWorldMaterix * go->GetTransform()->getLocalMatrix();
	for (auto children : go->child) {
		UpdateTransform(children);
	}
	//updateMeshTransform();
}

void Scene::DrawObjects(Shader& shader) {
	// 该接口主要供 depth/shadow pass 使用，只设置 model matrix，不绑定材质。
	shader.use();
	for (auto go : renderableObjects) {
		for (auto mesh : go->meshes) {
			shader.setMat4("model", go->GetTransform()->worldMaterix);
			mesh->draw();
		}
	}
}

void Scene::Render(Camera* camera, RenderGraph& rg) {

	
}

NAMESPACE_END
