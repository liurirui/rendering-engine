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
	Scene();
	~Scene() ;
	void addRenderable(Renderable* newMesh);
	void storeMeshes(Model* newModel);
	void createModel(const std::string& modelPath);
	void loadFloorTexture(const std::string& TexturePath);
	void updateMeshTransform();

	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);


	//Interface function
	void Start();
	void Update();
	void Render(Camera* camera, RenderGraph& rg);

	//mesh
	std::vector<Renderable*> Translucent;
	std::vector<Renderable*> Opaque;

	//model
	std::vector<Model*> model;

	//Renderer
	MeshRenderer* meshRenderer = nullptr;
	
	static std::string rootPath;

private:

};

NAMESPACE_END