#pragma once

#include <Base/Constants.h>
#include<Base/TRefCountPtr.h>
#include <Base/Shader.h>
#include <RHI/RenderContext.h>
#include <array>
#include <memory>
class RenderGraph;
NAMESPACE_START
class RenderTarget;
class PostProcessRenderer {
public:
    explicit PostProcessRenderer(RenderContext& renderContext);
    ~PostProcessRenderer();
    virtual void render(RenderGraph& rg, FrameBufferInfo* sceneFBO, int effectNo);
    void resize(int width, int height);
    unsigned int getTargetColorTextureID(int  attachment,int effectNo );
    float time=0;
private:
    RenderTarget* targetForEffect(int effectNo);

    RenderContext& renderContext_;
    //shader
    TRefCountPtr<Shader> HightLightShader;
    TRefCountPtr<Shader> BlurShader;
    TRefCountPtr<Shader> DownSampleShader;
    TRefCountPtr<Shader> UpSampleShader;
    TRefCountPtr<Shader> RadialBlurShader;
    TRefCountPtr<Shader> MotionBlurShader;
    TRefCountPtr<Shader> BloomShader;
    TRefCountPtr<Shader> CartoonShader;
    TRefCountPtr<Shader> RippleShader;

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
