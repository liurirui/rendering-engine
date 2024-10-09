#include"Scene.h"
#include<iostream>
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




NAMESPACE_END