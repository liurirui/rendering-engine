#include"Renderable.h"
#include"Model.h"
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
    if (!mesh || !mesh->localBounds.isValid()) {
        boundingBoxCenter = glm::vec3(0.0f);
        return;
    }
    // 包围盒已经在模型导入时计算，实例无需为了中心点再次保留和遍历完整顶点数组。
    boundingBoxCenter = mesh->localBounds.center();
}

glm::vec3  Renderable::getWorldCenter(){
    return worldBoundingBoxCenter;
}
NAMESPACE_END
