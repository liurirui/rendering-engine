#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START
void Scene::addMesh(Mesh* newMesh) {
	if (newMesh == nullptr) {
		std::cout << "Cannot add a null mesh to the scene." << std::endl;
		return; 
	}
	if (newMesh->isTramslucent) {
		Translucent.push_back(newMesh);
	}
	else if(!newMesh->isTramslucent)Opaque.push_back(newMesh);
}

glm::vec3 Scene::calculateBoundingBoxCenter(Mesh* mesh) {
	glm::vec3 minVertex(std::numeric_limits<float>::max());
	glm::vec3 maxVertex(std::numeric_limits<float>::lowest());
	for (unsigned int i = 0; i < mesh->numVertex; ++i) {
		glm::vec3 pos = mesh->vertices[i].position;
		minVertex = glm::min(minVertex, pos);
		maxVertex = glm::max(maxVertex, pos);
	}
	glm::vec3 center = (minVertex + maxVertex) * 0.5f;
	return center;
}


float Scene::calculateDistance(glm::vec3 cameraPosition,glm::vec3 meshPosition) {
	return glm::length(cameraPosition - meshPosition);
}

// sort translucent meshes by their distance to the camera (far to near)
void Scene::sortTranslucentMeshes(glm::vec3 cameraPosition) {
	std::sort(Translucent.begin(), Translucent.end(),
		[cameraPosition, this](Mesh* a, Mesh* b) {
			glm::vec3 posA = transparentModel[a] * glm::vec4(calculateBoundingBoxCenter(a),1.0);
			glm::vec3 posB = transparentModel[b] * glm::vec4(calculateBoundingBoxCenter(b), 1.0);
			return calculateDistance(cameraPosition, posA) > calculateDistance(cameraPosition, posB);
		}
	);
}


NAMESPACE_END