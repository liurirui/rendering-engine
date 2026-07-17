#include"Renderable.h"
#include"Model.h"
#include <limits>
NAMESPACE_START
Renderable::Renderable() {}

Renderable::~Renderable() { 
    delete transform;
}

Renderable::Renderable(Mesh* meshNow, bool NowisTranslucentNow) {
	mesh = meshNow;
	isTranslucent = NowisTranslucentNow;
    //calculateCenter();
}

void Renderable::setTransformFromModel(Model* model) {
    if (!model || !model->model_go || !model->model_go->GetTransform()) {
        return;
    }
    *transform = *model->model_go->GetTransform();
}

void Renderable::calculateCenter() {
    glm::vec3 minVertex(std::numeric_limits<float>::max());
    glm::vec3 maxVertex(std::numeric_limits<float>::lowest());
    for (unsigned int i = 0; i < mesh->numVertex; ++i) {
        glm::vec3 pos = mesh->vertices[i].position;
        minVertex = glm::min(minVertex, pos);
        maxVertex = glm::max(maxVertex, pos);
    }
    boundingBoxCenter = (minVertex + maxVertex) * 0.5f;
}

glm::vec3  Renderable::getWorldCenter(){
    return worldBoundingBoxCenter;
}
NAMESPACE_END