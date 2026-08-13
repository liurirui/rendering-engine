#pragma once

#include <Base/Constants.h>
#include <Base/Shader.h>
#include <RHI/RenderContext.h>
#include <array>
#include <memory>
class RenderGraph;
NAMESPACE_START
class RenderTarget;
class AssetManager;
class PostProcessRenderer {
public:
    PostProcessRenderer(RenderContext& renderContext, AssetManager& assetManager);
    ~PostProcessRenderer();
    virtual void render(RenderGraph& rg, FrameBufferInfo* sceneFBO, int effectNo);
    void resize(int width, int height);
    unsigned int getTargetColorTextureID(int  attachment,int effectNo );
    float time=0;
private:
    RenderTarget* targetForEffect(int effectNo);

    RenderContext& renderContext_;
    AssetManager& assetManager_;
    //shader
    std::shared_ptr<Shader> HightLightShader;
    std::shared_ptr<Shader> BlurShader;
    std::shared_ptr<Shader> DownSampleShader;
    std::shared_ptr<Shader> UpSampleShader;
    std::shared_ptr<Shader> RadialBlurShader;
    std::shared_ptr<Shader> MotionBlurShader;
    std::shared_ptr<Shader> BloomShader;
    std::shared_ptr<Shader> CartoonShader;
    std::shared_ptr<Shader> RippleShader;

    // Render targets own all post-process attachment textures.
    static const int bloomLevel = 5;
    std::unique_ptr<RenderTarget> highLightTarget_;
    std::array<std::unique_ptr<RenderTarget>, bloomLevel> upSampleTargets_;
    std::array<std::unique_ptr<RenderTarget>, bloomLevel> downSampleTargets_;
    std::unique_ptr<RenderTarget> bloomTarget_;
    std::unique_ptr<RenderTarget> radialTarget_;
    std::unique_ptr<RenderTarget> motionTargetA_;
    std::unique_ptr<RenderTarget> motionTargetB_;
    std::unique_ptr<RenderTarget> cartoonTarget_;
    std::unique_ptr<RenderTarget> rippleTarget_;
    RenderTarget* nowTarget_ = nullptr;

    GraphicsPipeline PostProcessRenderer_graphicsPipeline;
    DepthStencilState PostProcessRenderer_depthStencilState;

    Texture2D* bloomTexture = nullptr;
    Texture2D* lastTexture=nullptr;

    bool firstRender = true;
    bool useFramebufferA = true;

    unsigned int VBO = 0, quadVAO = 0;
    float quadVertices[24] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
};

NAMESPACE_END
