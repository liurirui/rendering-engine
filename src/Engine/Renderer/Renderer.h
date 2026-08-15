#pragma once

#include <Base/Constants.h>
#include <Renderer/RenderGraph/RenderGraph.h>
#include <memory>

NAMESPACE_START

class AssetManager;
class Camera;
class MeshRenderer;
class PostProcessRenderer;
class RenderContext;
class Scene;

class Renderer {
public:
    Renderer(RenderContext& renderContext, AssetManager& assetManager);
    ~Renderer();

    void render(Scene& scene, Camera& camera, int postProcessEffect);
    void resize(int width, int height);

    MeshRenderer& getMeshRenderer();
    PostProcessRenderer& getPostProcessRenderer();
    const RenderGraph::Stats& getLastRenderGraphStats() const { return lastRenderGraphStats_; }

private:
    RenderContext& renderContext_;
    AssetManager& assetManager_;
    std::unique_ptr<MeshRenderer> meshRenderer_;
    std::unique_ptr<PostProcessRenderer> postProcessRenderer_;
    RenderGraph::Stats lastRenderGraphStats_;
};

NAMESPACE_END
