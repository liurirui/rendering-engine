#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START

std::string Scene::rootPath = "";

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

Scene::~Scene() {
	for (Renderable* renderable : Translucent) {
		delete renderable;
	}
	for (Renderable* renderable : Opaque) {
		delete renderable;
	}
}

NAMESPACE_END