#pragma once

#include <Base/Constants.h>
#include<Base/TRefCountPtr.h>
#include <RHI/RenderContext.h>
class RenderGraph;
NAMESPACE_START
class PostProcessRenderer {
public:
    PostProcessRenderer();
    ~PostProcessRenderer();
    virtual void render(RenderGraph& rg, FrameBufferInfo* sceneFBO);
    unsigned int getTargetColorTextureID(int  attachment,int effectNo );
    float time=0;
private:
    //shader
    TRefCountPtr<Shader> HightLightShader;
    TRefCountPtr<Shader> BlurShader;
    TRefCountPtr<Shader> RadialBlurShader;
    TRefCountPtr<Shader> MotionBlurShader;
    TRefCountPtr<Shader> BloomShader;
    TRefCountPtr<Shader> CartoonShader;
    TRefCountPtr<Shader> RippleShader;

    //fbo
    FrameBufferInfo HighLightFramebuffer;
    FrameBufferInfo PingpongFramebuffer[2];
    FrameBufferInfo BloomFramebuffer;
    FrameBufferInfo RadialFramebuffer;
    FrameBufferInfo MotionFramebufferA;
    FrameBufferInfo MotionFramebufferB;
    FrameBufferInfo CartoonFramebuffer;
    FrameBufferInfo RippleFramebuffer;
    FrameBufferInfo* NowFramebuffer;
    FrameBufferInfo* useFrameBufferInfo;

    GraphicsPipeline PostProcessRenderer_graphicsPipeline;
    DepthStencilState PostProcessRenderer_depthStencilState;

    //Texture
    Texture2D* fboBrightTexture = nullptr;
    Texture2D* fbopingpongColorTexture[2];
    Texture2D* fboRadialTexture = nullptr;
    Texture2D* fboMotionTextureA = nullptr;
    Texture2D* fboMotionTextureB = nullptr;
    Texture2D* fboBloomTexture = nullptr;
    Texture2D* fboCartoonTexture = nullptr;
    Texture2D* fboRippleTexture = nullptr;
    Texture2D* bloomTexture = nullptr;
    Texture2D* lastTexture=nullptr;

    bool firstRender = true;
    bool useFramebufferA = true;

    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

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