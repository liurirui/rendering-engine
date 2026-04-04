#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include "Mesh.h"
#include"Model.h"
#include "Transform.h"
#include <Base/Material.h>
NAMESPACE_START
class Renderable {
public:
    Renderable();
    ~Renderable();

    //initialization
    Renderable(Mesh* mesh, bool isTranslucent);

    //Assign the model's Transform to the Renderable
    void setTransformFromModel(Model* model);

    //Calculate the bounding box center
    void calculateCenter();
    //Get the coordinates of the center of the bounding box after matrix transformation 
    glm::vec3 getWorldCenter();
    glm::vec3 boundingBoxCenter = glm::vec3(0.0f, 0.0f, 0.0f);     //The center coordinates of the bounding box
    bool needCaculateWorldCenter = true;
    glm::vec3 worldBoundingBoxCenter = glm::vec3(0.0f, 0.0f, 0.0f);

    Mesh* mesh;
    Transform* transform = new Transform();
    bool isTranslucent = false;
    int modelNumber = 0;
}; 
NAMESPACE_END