#include"Renderable.h"
NAMESPACE_START
Renderable::Renderable() {}

Renderable::~Renderable() { delete mesh; }

Renderable::Renderable(Mesh* meshNow, bool NowisTranslucentNow) {
	mesh = meshNow;
	isTranslucent = NowisTranslucentNow;
    calculateCenter();
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
    if (needCaculateWorldCenter) {
        worldBoundingBoxCenter = transform.modelMatrix * glm::vec4(boundingBoxCenter, 1.0);
        needCaculateWorldCenter = false;
    }
    return worldBoundingBoxCenter;
}
NAMESPACE_END