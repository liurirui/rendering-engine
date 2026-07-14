#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START
Scene::~Scene() {
	for (auto light : lights) {
		delete light;
	}
	lights.clear();
	mainDirectionalLight = nullptr;

	for (auto model : models) {
		delete model;
	}
	models.clear();

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
	if (!go->meshes.empty()) renderableObjects.emplace_back(go);
	for (auto children : go->child) {
		storeObjectMeshes(children);
	}
}

void Scene::createModel(const std::string& modelPath){
	Model* nowModel= new Model(modelPath);
	if (!nowModel->model_go) {
		delete nowModel;
		return;
	}
	models.emplace_back(nowModel);
	addRootChild(nowModel->model_go);
	storeObjectMeshes(nowModel->model_go);
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
			// ´ýÌí¼Ó
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

void Scene::RenderObject() {
	for (auto go : renderableObjects) {
		for (auto mesh : go->meshes) {
			mesh->material->setUniform();
			mesh->material->shader.setMat4("model", go->GetTransform()->worldMaterix);
			mesh->draw();
		}
	}
}

void Scene::Render(Camera* camera, RenderGraph& rg) {

	
}

NAMESPACE_END