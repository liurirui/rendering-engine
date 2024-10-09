#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include "Mesh.h"
NAMESPACE_START
class Scene {
public:
	Scene() {};
	~Scene() {};
	void addMesh(Mesh* newMesh);
	std::vector<Mesh*> Translucent;
	std::vector<Mesh*> Opaque;

private:
	

};



NAMESPACE_END