#include "Renderer.h"

#include "MeshRenderer.h"
#include "PostProcessRenderer.h"
#include "RenderGraph/RenderGraph.h"
#include <Base/AssetManager.h>
#include <Base/Camera.h>
#include <Base/Scene.h>
#include <glad.h>

NAMESPACE_START

Renderer::Renderer(RenderContext& renderContext, AssetManager& assetManager)
    : renderContext_(renderContext), assetManager_(assetManager) {
    meshRenderer_.reset(new MeshRenderer(assetManager_));
    meshRenderer_->setFloorTexture(assetManager_.loadTexture2D("resources/textures/wood.png"));
    postProcessRenderer_.reset(new PostProcessRenderer());
}

Renderer::~Renderer() = default;

void Renderer::render(Scene& scene, Camera& camera, int postProcessEffect) {
    RenderGraph renderGraph;

    scene.Update();
    meshRenderer_->render(scene, &camera, renderGraph);

    if (postProcessEffect != 0) {
        postProcessRenderer_->render(renderGraph, meshRenderer_->getTargetFrameBuffer(), postProcessEffect);
    }

    renderGraph.execute(&renderContext_);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
}

MeshRenderer& Renderer::getMeshRenderer() {
    return *meshRenderer_;
}

PostProcessRenderer& Renderer::getPostProcessRenderer() {
    return *postProcessRenderer_;
}

NAMESPACE_END
