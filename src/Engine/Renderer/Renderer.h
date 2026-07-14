#pragma once

#include <Base/Constants.h>
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

    MeshRenderer& getMeshRenderer();
    PostProcessRenderer& getPostProcessRenderer();

private:
    RenderContext& renderContext_;
    AssetManager& assetManager_;
    std::unique_ptr<MeshRenderer> meshRenderer_;
    std::unique_ptr<PostProcessRenderer> postProcessRenderer_;
};

NAMESPACE_END
