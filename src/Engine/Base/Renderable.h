#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include "Mesh.h"
#include "Transform.h"
NAMESPACE_START
class Renderable {
public:
    Renderable();
    ~Renderable();

    //initialization
    Renderable(Mesh* mesh, bool isTranslucent);

    //Calculate the bounding box center
    void calculateCenter();

    //Get the coordinates of the center of the bounding box after matrix transformation 
    glm::vec3 getWorldCenter();

    Mesh* mesh;
    Transform transform;
    bool isTranslucent = false;
    int modelNumber = 0;
    glm::vec3 boundingBoxCenter=glm::vec3(0.0f,0.0f,0.0f);     //The center coordinates of the bounding box
    bool needCaculateWorldCenter = true;
    glm::vec3 worldBoundingBoxCenter = glm::vec3(0.0f, 0.0f, 0.0f);

}; 
NAMESPACE_END