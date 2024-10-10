#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Mesh.h"
NAMESPACE_START
class Scene {
public:
	Scene() {};
	~Scene() {};
	void addMesh(Mesh* newMesh);
	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);
	std::vector<Mesh*> Translucent;
	std::vector<Mesh*> Opaque;

	//transparent meshes' model matrix
	std::unordered_map<Mesh*, glm::mat4> transparentModel;

	//Calculate the bounding box center
	glm::vec3 calculateBoundingBoxCenter(Mesh* mesh);

private:
	

};



NAMESPACE_END