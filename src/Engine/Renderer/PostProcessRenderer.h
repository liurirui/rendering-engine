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
    virtual void render(RenderGraph& rg, Texture2D* sceneTexture);
    unsigned int getTargetColorTextureID(int  attachment);

private:
    //shader
    TRefCountPtr<Shader> HightLightShader;
    TRefCountPtr<Shader> BlurShader;
    TRefCountPtr<Shader> RadialBlurShader;
    TRefCountPtr<Shader> MotionBlurShader;
    TRefCountPtr<Shader> BloomShader;
    TRefCountPtr<Shader> CartoonShader;

    //fbo
    FrameBufferInfo HightLightFramebuffer;
    FrameBufferInfo PingpongFramebuffer[2];
    FrameBufferInfo BloomFramebuffer;
    FrameBufferInfo RadialFramebuffer;
    FrameBufferInfo MotionFramebuffer;
    FrameBufferInfo CartoonFramebuffer;

    GraphicsPipeline PostProcessRenderer_graphicsPipeline;
    DepthStencilState PostProcessRenderer_depthStencilState;

    //Texture
    Texture2D* fboBrightTexture = nullptr;
    Texture2D* fbopingpongColorTexture[2];
    Texture2D* fboRadialTexture = nullptr;
    Texture2D* fboMotionTexture = nullptr;
    Texture2D* fboBloomTexture = nullptr;
    Texture2D* fboCartoonTexture = nullptr;

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