#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START

std::string Scene::rootPath = "";
Scene:: Scene() {
	loadTexture();
};
void Scene::addRenderable(Renderable* newRenderable) {
	if (newRenderable == nullptr) {
		std::cout << "Cannot add a null Renderable to the scene." << std::endl;
		return; 
	}
	if (newRenderable->isTranslucent) {
		Translucent.push_back(newRenderable);
	}
	else if(!newRenderable->isTranslucent)Opaque.push_back(newRenderable);
}

void Scene::loadTexture() {
	ColorTextureMap["hands"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/hand_dif.png").c_str());
	ColorTextureMap["Visor"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/glass_dif.png").c_str());
	ColorTextureMap["Body"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/body_dif.png").c_str());
	ColorTextureMap["Helmet"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/helmet_diff.png").c_str());
	ColorTextureMap["Legs"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/leg_dif.png").c_str());
	ColorTextureMap["Arms"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/arm_dif.png").c_str());
	ColorTextureMap["glass"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/textures/skybox/back.jpg").c_str());
	ColorTextureMap["app"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/textures/background.jpg").c_str());
	ColorTextureMap["plane"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/textures/wood.png").c_str());
	ColorTextureMap["Mars_Cube.002"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/planet/mars.png").c_str());
	ColorTextureMap["Cube"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/rock/rock.png").c_str());
	ColorTextureMap["Backpack"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/backpack/diffuse.jpg").c_str());
	ColorTextureMap["Cyborg"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/cyborg/cyborg_diffuse.png").c_str());
	ColorTextureMap["VampireMesh"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/vampire/textures/Vampire_diffuse.png").c_str());
}

float Scene::calculateDistance(glm::vec3 cameraPosition,glm::vec3 meshPosition) {
	return glm::length(cameraPosition - meshPosition);
}

// sort translucent meshes by their distance to the camera (far to near)
void Scene::sortTranslucentMeshes(glm::vec3 cameraPosition) {
	std::sort(Translucent.begin(), Translucent.end(),
		[cameraPosition, this](Renderable* a, Renderable* b) {
			return calculateDistance(cameraPosition, a->getWorldCenter()) > calculateDistance(cameraPosition, b->getWorldCenter());
		}
	);
}

void Scene::storeMeshes(Model* newModel) {
	for(Mesh* mesh:newModel->meshes){
		//Storing the model's mesh
		Renderable* renderable = nullptr;
		string meshName = mesh->nowName;
		if (mesh->nowName == "frame" || mesh->nowName == "glass_2") {
			if (mesh->nowName == "frame")  renderable = new Renderable(mesh, false);
			else  renderable = new Renderable(mesh, true);
			renderable->modelNumber = 1;
			renderable->transform.setTransform(glm::vec3(3.0f, 1.0f, -0.8f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
		}
		else if (mesh->nowName == "mask") {
			renderable = new Renderable(mesh, true);
			renderable->modelNumber = 2;
			renderable->transform.setTransform(glm::vec3(3.0f, 1.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(25.0f, 25.0f, 25.0f));
		}
		else if (mesh->nowName == "hands" || mesh->nowName == "Visor" || mesh->nowName == "Body" ||
			mesh->nowName == "Helmet" || mesh->nowName == "Legs" || mesh->nowName == "Arms") {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 3;
			renderable->transform.setTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
		}
		else if (mesh->nowName=="Mars_Cube.002") {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 4;
			renderable->transform.setTransform(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
		}
		else if (mesh->nowName == "Cube") {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 5;
			renderable->transform.setTransform(glm::vec3(-3.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
		}
		else if (mesh->nowName == "Cyborg") {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 6;
			renderable->transform.setTransform(glm::vec3(-3.0f,0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		}
		else if (mesh->nowName == "VampireMesh") {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 7;
			renderable->transform.setTransform(glm::vec3(1.0f, 0.0f, -6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.035f, 0.035f, 0.035f));
		}
		else {
			renderable = new Renderable(mesh, false);
			renderable->modelNumber = 8;
			renderable->transform.setTransform(glm::vec3(3.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.75f, 0.75f, 0.75f));
		}
		if (renderable) {
			addRenderable(renderable);
		}
	}
}

void Scene::createModel(const std::string& modelPath){
	storeMeshes(new Model(modelPath));
}

Scene::~Scene() {
	for (Renderable* renderable : Translucent) {
		delete renderable;
	}
	for (Renderable* renderable : Opaque) {
		delete renderable;
	}
}

NAMESPACE_END