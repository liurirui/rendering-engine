#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Mesh.h"
#include"Renderable.h"
NAMESPACE_START
class Scene {
public:
	Scene() {};
	~Scene() ;
	void addRenderable(Renderable* newMesh);
	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);
	std::vector<Renderable*> Translucent;
	std::vector<Renderable*> Opaque;

	static std::string rootPath;

private:

};

NAMESPACE_END