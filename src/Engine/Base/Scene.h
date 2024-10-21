#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include<string>
#include "Mesh.h"
#include"Model.h"
#include"Renderable.h"
NAMESPACE_START
class Scene {
public:
	Scene();
	~Scene() ;
	void addRenderable(Renderable* newMesh);
	float calculateDistance(glm::vec3 cameraPosition, glm::vec3 meshPosition);
	void sortTranslucentMeshes(glm::vec3 cameraPosition);
	void storeMeshes(Model* newModel);
	void createModel(const std::string& modelPath);
	void loadTexture();
	void loadFloorTexture(const std::string& TexturePath);

	//mesh
	std::vector<Renderable*> Translucent;
	std::vector<Renderable*> Opaque;


	//texture
	unordered_map<std::string, Texture2D*> ColorTextureMap;
	Texture2D* floor = nullptr;

	static std::string rootPath;

	int modelNo=1;                 //模型序号，记录网格属于哪个模型，便于绑定对应变换矩阵

private:

};

NAMESPACE_END