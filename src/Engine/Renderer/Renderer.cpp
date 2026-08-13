#include "Renderer.h"

#include "MeshRenderer.h"
#include "PostProcessRenderer.h"
#include "RenderGraph/RenderGraph.h"
#include <Base/AssetManager.h>
#include <Base/Camera.h>
#include <Base/Logger.h>
#include <Base/Scene.h>
#include <glad.h>

NAMESPACE_START

Renderer::Renderer(RenderContext& renderContext, AssetManager& assetManager)
    : renderContext_(renderContext), assetManager_(assetManager) {
    meshRenderer_.reset(new MeshRenderer(renderContext_, assetManager_));
    meshRenderer_->setFloorTexture(assetManager_.loadTexture2D("resources/textures/wood.png"));
    postProcessRenderer_.reset(new PostProcessRenderer(renderContext_, assetManager_));
}

Renderer::~Renderer() = default;

void Renderer::render(Scene& scene, Camera& camera, int postProcessEffect) {
    // 每帧由 Renderer 组织完整流程；Example 不再直接拼接 shadow、scene 和 post pass。
    // 500ms 节流检查外置 Shader 文件；成功时原子替换 program，失败保留上一版。
    assetManager_.getShaderLibrary().updateHotReload();
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

void Renderer::resize(int width, int height) {
    // 窗口尺寸变化由这里统一分发，确保投影矩阵和所有 RenderTarget 同步更新。
    if (width <= 0 || height <= 0) {
        return;
    }
    const bool sizeChanged = renderContext_.windowsWidth != width || renderContext_.windowsHeight != height;
    renderContext_.windowsWidth = width;
    renderContext_.windowsHeight = height;
    meshRenderer_->resize(width, height);
    postProcessRenderer_->resize(width, height);
    if (sizeChanged) {
        Logger::Info("Renderer resized. width=" + std::to_string(width) + ", height=" + std::to_string(height));
    }
}

MeshRenderer& Renderer::getMeshRenderer() {
    return *meshRenderer_;
}

PostProcessRenderer& Renderer::getPostProcessRenderer() {
    return *postProcessRenderer_;
}

NAMESPACE_END
